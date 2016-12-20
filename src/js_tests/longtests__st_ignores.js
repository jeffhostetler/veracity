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

//////////////////////////////////////////////////////////////////
// These two tests once lived in stSmokeTests.js 
// They were relocated here because non-functional "ignores" were
// causing them to fail.  Failure of these tests risked masking
// other more serious failures that might occur.
//
// When "ignores" is fixed, these can go back to stSmokeTest, or
// they can just stay here.  But when that happens, there are a 
// couple lines in stUpdateTestsWithDirtyWD.js that should be
// un-commented.  The lines are marked with: IGNORES_FAILING
//////////////////////////////////////////////////////////////////

function st_ignores() 
{
	// Helper functions are in update_helpers.js
	// There is a reference list at the top of the file.
	// After loading, you must call initialize_update_helpers(this)
	// before you can use the helpers.
	load("update_helpers.js");			// load the helper functions
	initialize_update_helpers(this);	// initialize helper functions

    this.testSimpleIgnores = function testSimpleIgnores() {
        var o;
        var ignoreSuffix = ".ignorable";
        var ignorePattern = "*" + ignoreSuffix;
        var base = createFolderOnDisk('testSimpleIgnores');
        addRemoveAndCommit();
        var folder = createFolderOnDisk(pathCombine(base, "folder" + ignoreSuffix));
        var file = createFileOnDisk(pathCombine(folder, "file" + ignoreSuffix), 20);
        
        // folder and folder/file should show up as Adds.  We'll just check for anything...
        vscript_test_wc__statusDirty();
        // now set an ignores for ignoreSuffix
        o = sg.exec("vv", "localsetting", "add-to", "ignores", ignorePattern);
        if (o.exit_status != 0)
        {
            print(sg.to_json(o));
            testlib.ok( (0), '## Unable to add "' + ignorePattern + '" to localsetting ignores.' );
            print(sg.exec("vv", "localsettings").stdout);
            return false;
        }
        // and see if it worked
	vscript_test_wc__statusEmptyExcludingIgnores();
        dumpTree("Should be ignoring: " + ignorePattern );

        // status of controlled files should be unaffected, however
        o = sg.exec("vv", "localsetting", "remove-from", "ignores", ignorePattern);
        if (o.exit_status != 0) 
        {
            print(sg.to_json(o));
            testlib.ok( (0), '## Unable to remove "' + ignorePattern + '" from localsetting ignores.' );
            return false;
        }
        addRemoveAndCommit();
        o = sg.exec("vv", "localsetting", "add-to", "ignores", ignorePattern);
        if (o.exit_status != 0)
        {
            print(sg.to_json(o));
            testlib.ok( (0), '## Unable to add "' + ignorePattern + '" to localsetting ignores.' );
            print(sg.exec("vv", "localsettings").stdout);
            return false;
        }
        doRandomEditToFile(file);
        vscript_test_wc__statusDirty();

        // ok. now clean up
        commit();
        o = sg.exec("vv", "localsetting", "remove-from", "ignores", ignorePattern);
        if (o.exit_status != 0) 
        {
            print(sg.to_json(o));
            testlib.ok( (0), '## Unable to remove "' + ignorePattern + '" from localsetting ignores.' );
            return false;
        }
        vscript_test_wc__statusEmpty();
    }

///////////////////////////////////////////////////////////////////////////////////////////////////

    this.ignores = function(cmd, value) {
        var o;
        if (cmd == "reset") {
            o = sg.exec("vv", "localsetting", "reset", "ignores");
            if (o.exit_status != 0) 
            {
                print(sg.to_json(o));
                testlib.ok( (0), '## Unable to reset localsetting ignores.' );
                return false;
            }
        } else {
            o = sg.exec("vv", "localsetting", cmd, "ignores", value);
            if (o.exit_status != 0)
            {
                print(sg.to_json(o));
                testlib.ok( (0), '## Ignores: unable to ' + cmd + '"' + value + '"' );
                print(sg.exec("vv", "localsettings").stdout);
                return false;
            }
        }
        return true;
    }

    this.testDeeperIgnores = function testDeeperIgnores() {
        // this is a much more thorough test of ignores,
        // verifying that ignores don't apply to version-controlled items
        var xf = ".iFile";
		var pf = "*" + xf;
        var xd = ".iDir";
		var pd = "*" + xd;
        var tree_status;

        // start with a clean set of ignores
        this.ignores("reset");

        var base = createFolderOnDisk('testDeeperIgnores');
        addRemoveAndCommit();

        // ignores ON
        this.ignores("add-to", pf);
        this.ignores("add-to", pd);

        var mod_1 = createFileOnDisk(pathCombine(base, "mod_1" + xf), 20);
        var mov_1 = createFileOnDisk(pathCombine(base, "mov_1" + xf), 20);
        var ren_1 = createFileOnDisk(pathCombine(base, "ren_1" + xf), 20);
        var del_1 = createFileOnDisk(pathCombine(base, "del_1" + xf), 20);
        var dir_1 = createFolderOnDisk(pathCombine(base, "dir_1" + xd));
        var mod_11 = createFileOnDisk(pathCombine(dir_1, "mod_11" + xf), 20);
        var mov_11 = createFileOnDisk(pathCombine(dir_1, "mov_11" + xf), 20);
        var ren_11 = createFileOnDisk(pathCombine(dir_1, "ren_11" + xf), 20);
        var del_11 = createFileOnDisk(pathCombine(dir_1, "del_11" + xf), 20);
        var dir_A = createFolderOnDisk(pathCombine(base, "dir_A" + xd));
        var mod_A1 = createFileOnDisk(pathCombine(dir_A, "mod_A1" + xf), 20);
        var mov_A1 = createFileOnDisk(pathCombine(dir_A, "mov_A1" + xf), 20);
        var ren_A1 = createFileOnDisk(pathCombine(dir_A, "ren_A1" + xf), 20);
        var del_A1 = createFileOnDisk(pathCombine(dir_A, "del_A1" + xf), 20);
        var dir_Dest = createFolderOnDisk(pathCombine(base, "dir_Dest" + xd));

        expected = new Array;
        expected["Ignored"] = [ "@/" + mod_1 ,
                                "@/" + mov_1 ,
                                "@/" + ren_1 ,
                                "@/" + del_1 ,
                                "@/" + dir_1 ,
                                // "@/" + mod_11 ,	// With changes for W6508, we don't dive into ignored directories.
                                // "@/" + mov_11 ,
                                // "@/" + ren_11 ,
                                // "@/" + del_11 ,
                                "@/" + dir_A ,
                                // "@/" + mod_A1 ,
                                // "@/" + mov_A1 ,
                                // "@/" + ren_A1 ,
                                // "@/" + del_A1 ,
                                "@/" + dir_Dest ];
	vscript_test_wc__confirm(expected);

        // ignores OFF
        this.ignores("reset");

		// We cancelled the suffix ignores, so all of the dirt we created
		// should now appear as FOUND items.

        expected = new Array;
        expected["Found"] = [ "@/" + mod_1 ,
                              "@/" + mov_1 ,
                              "@/" + ren_1 ,
                              "@/" + del_1 ,
                              "@/" + dir_1 ,
                              "@/" + mod_11 ,
                              "@/" + mov_11 ,
                              "@/" + ren_11 ,
                              "@/" + del_11 ,
                              "@/" + dir_A ,
                              "@/" + mod_A1 ,
                              "@/" + mov_A1 ,
                              "@/" + ren_A1 ,
                              "@/" + del_A1 ,
                              "@/" + dir_Dest ];
        vscript_test_wc__confirm(expected);

		// ADDREMOVE will convert the FOUNDs to ADDEDs.
        vscript_test_wc__addremove();

		// Add the suffixes back to the IGNORES list and
        // confirm that STATUS does not filter out ADDED items.
		// That is, only stuff listed in FOUND should be affected,
		// not stuff listed in ADDED.
        this.ignores("add-to", pf);
        this.ignores("add-to", pd);
        expected = new Array;
        expected["Added"] = [ "@/" + mod_1 ,
                              "@/" + mov_1 ,
                              "@/" + ren_1 ,
                              "@/" + del_1 ,
                              "@/" + dir_1 ,
                              "@/" + mod_11 ,
                              "@/" + mov_11 ,
                              "@/" + ren_11 ,
                              "@/" + del_11 ,
                              "@/" + dir_A ,
                              "@/" + mod_A1 ,
                              "@/" + mov_A1 ,
                              "@/" + ren_A1 ,
                              "@/" + del_A1 ,
                              "@/" + dir_Dest ];
        vscript_test_wc__confirm(expected);

		// Commit all of the ADDED stuff.  This implicitly tests that COMMIT
		// properly limits ignores.

        commit();

		// Verify that COMMIT got everything that we ADDED.
        expected = new Array;
        vscript_test_wc__confirm(expected);

		// Just to be safe, also verify that there is no other dirt.
        expected = new Array;
        vscript_test_wc__confirm_wii(expected);

		////////////////////////////////////////////////////////////////
        // Now, mod, move, rename, remove, add, etc.
		// This is testing that each of these command verbs
		// do not get confused and try to ignore items under
		// version control.

        var mod_1_x = mod_1;
        doRandomEditToFile(mod_1);
        var mov_1_x = pathCombine(dir_Dest, getFileNameFromPath(mov_1));
        vscript_test_wc__move(mov_1, dir_Dest);
        var ren_1_x = pathCombine(getParentPath(ren_1), "ren_NEW" + xf);
        vscript_test_wc__rename(ren_1, "ren_NEW" + xf);
        var del_1_x = del_1;
        vscript_test_wc__remove_file(del_1);
        var add_1_x = createFileOnDisk(pathCombine(base, "add_1" + xf), 20);

        var mod_11_x = mod_11;
        doRandomEditToFile(mod_11);
        var mov_11_x = pathCombine(dir_Dest, getFileNameFromPath(mov_11));
        vscript_test_wc__move(mov_11, dir_Dest);
        var ren_11_x = pathCombine(getParentPath(ren_11), "ren_NEW" + xf);
        vscript_test_wc__rename(ren_11, "ren_NEW" + xf);
        var del_11_x = del_11;
        vscript_test_wc__remove_file(del_11);
        var add_11_x = createFileOnDisk(pathCombine(dir_1, "add_11" + xf), 20);
        var mod_A1_x = mod_A1;
        doRandomEditToFile(mod_A1);
        var mov_A1_x = pathCombine(dir_Dest, getFileNameFromPath(mov_A1));
        print("MOVE: " + mov_A1 + " to " + dir_Dest);
        vscript_test_wc__move(mov_A1, dir_Dest);
        var ren_A1_x = pathCombine(getParentPath(ren_A1), "ren_NEW" + xf);
        vscript_test_wc__rename(ren_A1, "ren_NEW" + xf);
        var del_A1_x = del_A1;
        vscript_test_wc__remove_file(del_A1);
        var add_A1_x = createFileOnDisk(pathCombine(dir_A, "add_A1" + xf), 20);

        var dir_B = pathCombine(getParentPath(dir_A), "dir_B");
        mod_A1_x = pathCombine(dir_B, getFileNameFromPath(mod_A1_x));
        ren_A1_x = pathCombine(dir_B, getFileNameFromPath(ren_A1_x));
        del_A1_x = pathCombine(dir_B, getFileNameFromPath(del_A1_x));
        add_A1_x = pathCombine(dir_B, getFileNameFromPath(add_A1_x));
        vscript_test_wc__rename(dir_A, "dir_B");

        // all items should be represented except the Found ones
        expected = new Array;
        expected["Ignored"]    = [ "@/" + add_1_x ,
                                   "@/" + add_11_x ,
                                   "@/" + add_A1_x ];
        expected["Removed"]  = [ "@b/" + del_1 ,	// removed items can only be
                                 "@b/" + del_11 ,	// addressed using the baseline
                                 "@b/" + del_A1 ];	// extended-prefix repo-path
        expected["Modified"] = [ "@/" + mod_1_x ,
                                 "@/" + mod_11_x ,
                                 "@/" + mod_A1_x ];
        expected["Renamed"]  = [ "@/" + ren_1_x ,
                                 "@/" + ren_11_x ,
                                 "@/" + ren_A1_x ,
                                 "@/" + dir_B ];
        expected["Moved"]    = [ "@/" + mov_1_x ,
                                 "@/" + mov_11_x ,
                                 "@/" + mov_A1_x ];
        vscript_test_wc__confirm(expected);
        commit();

        expected = new Array;
        expected["Ignored"]    = [ "@/" + add_1_x ,
                                   "@/" + add_11_x ,
                                   "@/" + add_A1_x ];
        vscript_test_wc__confirm(expected);

        // turn off the ignores ...
        this.ignores("reset");

        expected = new Array;
        expected["Found"]    = [ "@/" + add_1_x ,
                                 "@/" + add_11_x ,
                                 "@/" + add_A1_x ];
        vscript_test_wc__confirm(expected);

        addRemoveAndCommit();
    }

	this.testRootedIgnores = function testRootedIgnores() {
		sg.file.write(".vvignores", "@/ignoreFolder/**\n@/normalFolder/ignore*");
		sg.fs.mkdir_recursive("ignoreFolder");
		sg.fs.mkdir_recursive("normalFolder");
		createFileOnDisk("ignoreFolder/do_not_add.txt", 20);
		createFileOnDisk("normalFolder/ignoreMe.txt", 20);
		
		//Verify that status doesn't return the ignored file
		o = testlib.execVV("status");
		testlib.ok(-1 == o.stdout.indexOf("do_not_add.txt"));
		testlib.ok(-1 == o.stdout.indexOf("ignoreMe.txt"));
		testlib.ok(-1 != o.stdout.indexOf("normalFolder"));
		
		//Verify that add doesn't add the ignored file
		o = testlib.execVV("add",  ".");
		o = testlib.execVV("status");
		testlib.ok(-1 == o.stdout.indexOf("do_not_add.txt"));
		testlib.ok(-1 == o.stdout.indexOf("ignoreMe.txt"));
		testlib.ok(-1 != o.stdout.indexOf("normalFolder"));
		
// In the WC branch I don't yet have REVERT working.
// And besides, if the previous tests all passed, then
// there won't be anything to actually revert.
//		o = testlib.execVV("revert", "--all");
		
		//Verify that addremove doesn't add the ignored file
		o = testlib.execVV("addremove");
		o = testlib.execVV("status");
		testlib.ok(-1 == o.stdout.indexOf("do_not_add.txt"));
		testlib.ok(-1 == o.stdout.indexOf("ignoreMe.txt"));
		testlib.ok(-1 != o.stdout.indexOf("normalFolder"));
		
		//This should add normalFolder, but nothing else
		testlib.execVV("commit", "-m", "test");
		//make sure that the ignored files don't show up in a normal status
		o = testlib.execVV("status");
		testlib.ok(-1 == o.stdout.indexOf("do_not_add.txt"));
		testlib.ok(-1 == o.stdout.indexOf("ignoreMe.txt"));
		
		//make sure that the ignored files DO show up in a no-ignores status
		o = testlib.execVV("status", "--no-ignores");
		testlib.ok(-1 != o.stdout.indexOf("do_not_add.txt"));
		testlib.ok(-1 != o.stdout.indexOf("ignoreMe.txt"));
		
	}
}
