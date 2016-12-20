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
// Test PARTIAL COMMIT in the presence of MOVES/RENAMES where we
// can create an non-well-formed TN containing 2 items with the
// same name.  See W1976.
//////////////////////////////////////////////////////////////////

function st_partial_commit_W1976_001()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	// create a new repo and a fresh WD.

	var my_test_1_dir  = "test_1" + "_" + new Date().getTime();
	var my_test_1_repo = my_test_1_dir;
	var my_test_1_root = pathCombine(tempDir, my_test_1_dir);

	if (!sg.fs.exists(my_test_1_root))
	    this.do_fsobj_mkdir_recursive(my_test_1_root);
	this.do_fsobj_cd(my_test_1_root);
	sg.vv2.init_new_repo( { "repo" : my_test_1_repo, "hash" : "SHA1/160" } );
	whoami_testing( my_test_1_dir );

	//////////////////////////////////////////////////////////////////

	this.do_fsobj_mkdir("dir_A");
	this.do_fsobj_mkdir("dir_A/dir_B");
	this.do_fsobj_mkdir("dir_A/dir_B/dir_C");
        this.do_fsobj_create_file("dir_A/dir_B/dir_C/file_1");
        this.do_fsobj_create_file("dir_A/dir_B/dir_C/file_2");
	vscript_test_wc__addremove();
	this.do_commit("add initial files");
	var cset_base = sg.wc.parents()[0];
	var cset_head = "";

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Do some tricky renames....");
	print("");

	vscript_test_wc__rename("dir_A/dir_B/dir_C/file_1", "file_t");
	vscript_test_wc__rename("dir_A/dir_B/dir_C/file_2", "file_1");

	// We now have {file_t, file_1}.
	// Try to do a partial commit of the current file_1.
	// This should cause a conflict/issue because the
	// resulting TN/CSET should have 2 items with the 
	// same name.
	// [1] The original file_1 with its original name.
	// [2] The original file_2 with its new name.

	try
	{
	    vscript_test_wc__commit_np( { "src" : "dir_A/dir_B/dir_C/file_1",
					  "message" : "Partial commit of file_1" } );
	    testlib.ok( (0), "Partial commit succeeded, but was expecting an error." );
	    cset_head = sg.wc.parents()[0];
	}
	catch (e)
	{
	    print(dumpObj(e, "Expect hard error from this partial commit.", "", 0));
	    testlib.ok( (e.name == "Error"),
			"Expect hard error.");
	    testlib.ok( (e.message.search("Partial commit would cause collision: 'file_1'") >= 0),
			"Expect hard error.");
	}

	// since commit should have failed, these should still be dirty.

        var expect_test = new Array;
	expect_test["Renamed"] = [ "@/dir_A/dir_B/dir_C/file_t",
				   "@/dir_A/dir_B/dir_C/file_1"
				 ];
        vscript_test_wc__confirm_wii(expect_test);

	testlib.ok( (sg.fs.exists("dir_A/dir_B/dir_C/file_t")==true), "dir_A/dir_B/dir_C/file_t should exist.");
	testlib.ok( (sg.fs.exists("dir_A/dir_B/dir_C/file_1")==true), "dir_A/dir_B/dir_C/file_1 should exist.");

    }

}
