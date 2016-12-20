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

///////////////////////////////////////////////////////////////////////////////////////
// Tests involving updates between changesets with various patterns of path swapping.
// There are 10 tests here, any combination of which will fail on a given run.
// The failures indicate pathcycle problems.
///////////////////////////////////////////////////////////////////////////////////////

var globalThis;  // var to hold "this" pointers

function st_update_paths() 
{
    // Helper functions are in update_helpers.js
    // There is a reference list at the top of the file.
    // After loading, you must call initialize_update_helpers(this)
    // before you can use the helpers.
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        // we begin with a clean WD in a fresh REPO.
        // and build a sequence of changesets to use

        this.workdir_root = sg.fs.getcwd();        // save this for later

        //////////////////////////////////////////////////////////////////
        // fetch the HID of the initial commit.

        this.csets.push( sg.wc.parents()[0] );
        this.names.push( "*Initial Commit*" );

        //////////////////////////////////////////////////////////////////
        // simple (3-deep) path swapping setup for update tests

        this.do_fsobj_mkdir("dir_update_10");
        vscript_test_wc__addremove();
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("Update STEM 10");

        this.do_fsobj_mkdir("dir_update_10/a");
        this.do_fsobj_mkdir("dir_update_10/a/b");
        this.do_fsobj_mkdir("dir_update_10/a/b/c");
        vscript_test_wc__addremove();
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("Update HUB 10");

        vscript_test_wc__move("dir_update_10/a/b", "dir_update_10");
        vscript_test_wc__move("dir_update_10/a", "dir_update_10/b");
        vscript_test_wc__move("dir_update_10/b/c", "dir_update_10/b/a");
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("Update HEAD 1 b/a/c");

        if (!this.do_update_when_clean_by_name("Update STEM 10"))
            return;
        if (!this.do_update_when_clean_by_name("Update HUB 10"))
            return;
        vscript_test_wc__move("dir_update_10/a/b", "dir_update_10");
        vscript_test_wc__move("dir_update_10/a", "dir_update_10/b/c");
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("Update HEAD 2 b/c/a");

        if (!this.do_update_when_clean_by_name("Update STEM 10"))
            return;
        if (!this.do_update_when_clean_by_name("Update HUB 10"))
            return;
        vscript_test_wc__move("dir_update_10/a/b/c", "dir_update_10/a");
        vscript_test_wc__move("dir_update_10/a/b", "dir_update_10/a/c");
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("Update HEAD 3 a/c/b");

        if (!this.do_update_when_clean_by_name("Update STEM 10"))
            return;
        if (!this.do_update_when_clean_by_name("Update HUB 10"))
            return;
        vscript_test_wc__move("dir_update_10/a/b/c", "dir_update_10");
        vscript_test_wc__move("dir_update_10/a", "dir_update_10/c");
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("Update HEAD 4 c/a/b");

        if (!this.do_update_when_clean_by_name("Update STEM 10"))
            return;
        if (!this.do_update_when_clean_by_name("Update HUB 10"))
            return;
        vscript_test_wc__move("dir_update_10/a/b/c", "dir_update_10");
        vscript_test_wc__move("dir_update_10/a/b", "dir_update_10/c");
        vscript_test_wc__move("dir_update_10/a", "dir_update_10/c/b");
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("Update HEAD 5 c/b/a");

        this.list_csets("After setup (from update_helpers.generate_test_changesets())");
                 print("------------------------------------------------------");
    }

    this.update_up = function(toName)
    {
        this.force_clean_WD();
        this.do_update_when_clean_by_name("Update STEM 10");
        this.do_update_when_clean_by_name("Update HUB 10");
        testlib.ok( (this.do_update_when_clean_by_name(toName)), 'Update forward from "Update HUB 10" to "' + toName + '"');
        if (!vscript_test_wc__statusEmpty()) {
            dumpTree();
        }
        this.force_clean_WD();
    }

    this.test_update_up_1 = function()
    {
        this.update_up("Update HEAD 1 b/a/c");
    }

    this.test_update_up_2 = function()
    {
        this.update_up("Update HEAD 2 b/c/a");
    }

    this.test_update_up_3 = function()
    {
        this.update_up("Update HEAD 3 a/c/b");
    }

    this.test_update_up_4 = function()
    {
        this.update_up("Update HEAD 4 c/a/b");
    }

    this.test_update_up_5 = function()
    {
        this.update_up("Update HEAD 5 c/b/a");
    }

    ////////////////////////////////////////////////////////////////////

    this.update_down = function(fromName)
    {
        this.force_clean_WD();
        this.do_update_when_clean_by_name("Update STEM 10");
        this.do_update_when_clean_by_name(fromName);
        testlib.ok( (this.do_update_when_clean_by_name("Update HUB 10")), 'Update back from "' + fromName + '" to "Update HUB 10"');
        if (!vscript_test_wc__statusEmpty()) {
            dumpTree();
        }
        this.force_clean_WD();
    }

    this.test_update_down_1 = function()
    {
        this.update_down("Update HEAD 1 b/a/c");
    }

    this.test_update_down_2 = function()
    {
        this.update_down("Update HEAD 2 b/c/a");
    }

    this.test_update_down_3 = function()
    {
        this.update_down("Update HEAD 3 a/c/b");
    }

    this.test_update_down_4 = function()
    {
        this.update_down("Update HEAD 4 c/a/b");
    }

    this.test_update_down_5 = function()
    {
        this.update_down("Update HEAD 5 c/b/a");
    }


    this.tearDown = function() 
    {

    }
}
