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

/////////////////////////////////////////////////////////////////////////////////////
// Tests involving merges between changesets with various patterns of path swapping.
// Two of these tests cause merge to hang.  The remainder of the tests pass.
/////////////////////////////////////////////////////////////////////////////////////

var globalThis;  // var to hold "this" pointers

function st_merge_paths_00__W2664() 
{
    // Helper functions are in update_helpers.js
    // There is a reference list at the top of the file.
    // After loading, you must call initialize_update_helpers(this)
    // before you can use the helpers.
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.
    var my_names = new Array;

    //////////////////////////////////////////////////////////////////

    this.my_commit = function(name)
    {
	print("================================");
	print("Attempting COMMIT for: " + name);
	print("");

	sg.exec(vv, "branch", "attach", "master");
	var hid = vscript_test_wc__commit(name);
	my_names[name] = hid;
    }

    this.my_update = function(name)
    {
	print("================================");
	print("Attempting UPDATE to: " + name);
	print("");

	vscript_test_wc__update(my_names[name]);
    }
	
    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        // we begin with a clean WD in a fresh REPO.
        // and build a sequence of changesets to use

        this.workdir_root = sg.fs.getcwd();        // save this for later

        //////////////////////////////////////////////////////////////////
        // simple (3-deep) path swapping setup for merge tests

        this.do_fsobj_mkdir("dir_merge_10");
        vscript_test_wc__addremove();
        this.my_commit("Merge STEM 10");

        this.do_fsobj_mkdir("dir_merge_10/a");
        this.do_fsobj_mkdir("dir_merge_10/a/b");
        this.do_fsobj_mkdir("dir_merge_10/a/b/c");
        vscript_test_wc__addremove();
        this.my_commit("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/b");
        vscript_test_wc__move("dir_merge_10/b/c", "dir_merge_10/b/a");
	sg.exec(vv, "branch", "attach", "master");
        this.my_commit("Merge HEAD 1 b/a/c");

	this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/b/c");
        this.my_commit("Merge HEAD 2 b/c/a");

        this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b/c", "dir_merge_10/a");
        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10/a/c");

        this.my_commit("Merge HEAD 3 a/c/b");

        this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b/c", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/c");

        this.my_commit("Merge HEAD 4 c/a/b");

        this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b/c", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10/c");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/c/b");

        this.my_commit("Merge HEAD 5 c/b/a");

        this.list_csets("After setup (from update_helpers.generate_test_changesets())");
                 print("------------------------------------------------------");
    }

    this.my_merge = function(name)
    {
	print("================================");
	print("Attempting MERGE with: " + name);
	print("");

	vscript_test_wc__merge_np( { "rev" : my_names[name] } );
    }

////////////////////////////////////////////////////////////////////////////

    this.merge_sub = function(baseName, mergeName)
    {
	print("================================");
	print("Attempting MERGE_SUB with: " + baseName + " and " + mergeName);
	print("");

        this.assert_clean_WD();
	this.my_update("Merge STEM 10");
	this.my_update(baseName);
        this.my_merge(mergeName);
        this.force_clean_WD();
    }
    
    this.test_merge_1_2 = function test_merge_1_2()
    {
        this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 2 b/c/a");
    }

    this.test_merge_1_3 = function test_merge_1_3()
    {
        this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 3 a/c/b");
    }

    this.test_merge_1_4 = function test_merge_1_4()
    {
        this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 4 c/a/b");
    }

    this.test_merge_1_5 = function test_merge_1_5()
    {
        this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 5 c/b/a");
    }

////////////////////////////////////////////////////////////////////////////

    this.test_merge_2_1 = function test_merge_2_1()
    {
        this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 1 b/a/c");
    }

    this.test_merge_2_3 = function test_merge_2_3()
    {
        this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 3 a/c/b");
    }

    this.test_merge_2_4 = function test_merge_2_4()
    {
        this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 4 c/a/b");
    }

    this.test_merge_2_5 = function test_merge_2_5()
    {
        this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 5 c/b/a");
    }

////////////////////////////////////////////////////////////////////////////

    this.test_merge_3_1 = function test_merge_3_1()
    {
        this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 1 b/a/c");
    }

/*************************************************
 *** Prior to W2664: THIS TEST CAUSES MERGE TO HANG (1 of 2)  ***
 *************************************************/
    this.test_merge_3_2 = function test_merge_3_2()
    {
        print("##########################################");
        print("### test_merge_3_2 is expected to hang ...");
        print("##########################################");
        this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 2 b/c/a");
    }

/*************************************************
 *** Prior to W2664: THIS TEST CAUSES MERGE TO HANG (2 of 2)  ***
 *************************************************/
    this.test_merge_3_4 = function test_merge_3_4()
    {
        print("##########################################");
        print("### test_merge_3_4 is expected to hang ...");
        print("##########################################");
        this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 4 c/a/b");
    }

    this.test_merge_3_5 = function test_merge_3_5()
    {
        this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 5 c/b/a");
    }

////////////////////////////////////////////////////////////////////////////

    this.test_merge_4_1 = function test_merge_4_1()
    {
        this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 1 b/a/c");
    }

    this.test_merge_4_2 = function test_merge_4_2()
    {
        this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 2 b/c/a");
    }

    this.test_merge_4_3 = function test_merge_4_3()
    {
        this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 3 a/c/b");
    }

    this.test_merge_4_5 = function test_merge_4_5()
    {
       this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 5 c/b/a");
    }

////////////////////////////////////////////////////////////////////////////

    this.test_merge_5_1 = function test_merge_5_1()
    {
        this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 1 b/a/c");
    }

    this.test_merge_5_2 = function test_merge_5_2()
    {
        this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 2 b/c/a");
    }

    this.test_merge_5_3 = function test_merge_5_3()
    {
        this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 3 a/c/b");
    }

    this.test_merge_5_4 = function test_merge_5_4()
    {
        this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 4 c/a/b");
    }

    this.tearDown = function() 
    {

    }
}
