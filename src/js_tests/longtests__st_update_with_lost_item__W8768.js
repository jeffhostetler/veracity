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
// Tests to try to UNDO an ADD.
//////////////////////////////////////////////////////////////////

// Was: st_undo_added__P00015_04()
function st_update_with_lost_item__W8768()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

        this.do_fsobj_create_file("baseFile_a");
	vscript_test_wc__addremove();
	this.do_commit("add baseFile_a");
	var cset_base_a = sg.wc.parents()[0];

        this.do_fsobj_create_file("baseFile_b");
	vscript_test_wc__addremove();
	this.do_commit("add baseFile_b");
	var cset_base_b = sg.wc.parents()[0];

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Add items...");
	print("");

        this.do_fsobj_mkdir("dir_A");
        this.do_fsobj_create_file("file_0");
        vscript_test_wc__addremove();

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Use /bin/rm to DELETE the added file...");
	print("");

	this.do_fsobj_remove_file("file_0");
	this.do_fsobj_remove_dir("dir_A");

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/file_0" ];
	expect_test["Lost"] = [ "@/dir_A",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Try doing an UPDATE to the cset before we did anything...");
	print("");

	// the WD should still be at cset_base_b w/ dirt.

	try
	{
	    vscript_test_wc__update( cset_base_a );
	    testlib.ok( (0), "Expect error when doing UPDATE with LOST item." );
	}
	catch (e)
	{
	    print(dumpObj(e, "Expect hard error.", "", 0));
	    testlib.ok( (e.name == "Error"), "Expect hard error.");
	    testlib.ok( (e.message.search("Operation not allowed with a lost item") >= 0),
			"Expect hard error.");

	    // use existing "expect_test" to confirm that UPDATE didn't change anything.
            vscript_test_wc__confirm_wii(expect_test);
	}
    }
}
