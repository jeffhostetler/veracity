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

function st_ignores_status1__B7193() 
{
	// Helper functions are in update_helpers.js
	// There is a reference list at the top of the file.
	// After loading, you must call initialize_update_helpers(this)
	// before you can use the helpers.
	load("update_helpers.js");			// load the helper functions
	initialize_update_helpers(this);	// initialize helper functions

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
        var cset0 = addRemoveAndCommit();
        var baseX = createFolderOnDisk('xxxxx');
        var cset1 = addRemoveAndCommit();

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

		var found1 = createFileOnDisk(pathCombine(base, "found1"), 20);
		var found2 = createFileOnDisk(pathCombine(baseX, "found2"), 20);
		var founddir1 = createFolderOnDisk('founddir');
		var found3 = createFileOnDisk(pathCombine(founddir1, "found3"), 20);

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
		expected["Found"] = [ "@/" + found1,
							  "@/" + found2,
							  "@/" + founddir1,
							  "@/" + found3 ];
		print("vv status --verbose");
		print(sg.exec(vv,"status", "--verbose").stdout);
		vscript_test_wc__confirm(expected, sg.wc.status({}));

		print("vv status --verbose -r cset0");
		print(sg.exec(vv,"status", "--verbose", "-r", cset0).stdout);
		expected["Added"] = [ "@/xxxxx" ];
		vscript_test_wc__confirm(expected, sg.wc.status({"rev":cset0}));

        // ignores OFF
        this.ignores("reset");
    }
}
