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
// Tests to try to UNDO an ADD that has been partially deleted.
// See P00015 and R0621.
//////////////////////////////////////////////////////////////////

function st_undo_added_deleted__R0621()
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
	this.do_fsobj_mkdir("dir_A/dir_B");
	this.do_fsobj_mkdir("dir_A/dir_B/dir_C");
        this.do_fsobj_create_file("dir_A/dir_B/dir_C/file_1");
        this.do_fsobj_create_file("file_0");
        vscript_test_wc__addremove();

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/dir_A/dir_B",
				 "@/dir_A/dir_B/dir_C",
				 "@/dir_A/dir_B/dir_C/file_1",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Make some dirt by /bin/rm deleting something....");
	print("");

	this.do_fsobj_remove_dir_recursive("dir_A/dir_B");

        var expect_test = new Array;
	expect_test["Lost"] = [ "@/dir_A/dir_B",
				"@/dir_A/dir_B/dir_C",
				"@/dir_A/dir_B/dir_C/file_1"
			      ];
	expect_test["Added"] = [ "@/dir_A",
				 "@/dir_A/dir_B",
				 "@/dir_A/dir_B/dir_C",
				 "@/dir_A/dir_B/dir_C/file_1",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Verify that normal 'vv remove' converts LOST to REMOVED...");
	print("");

	var result = sg.exec(vv, "remove", "dir_A/dir_B");
	if (result.exit_status != 0)
	    print(dumpObj(result, "sg.exec()", "", 0));

	testlib.ok( (result.exit_status == 0), "'vv remove dir_A/dir_B' should pass" );

        var expect_test = new Array;
	// these WILL NOT be in the REMOVED section because they were UNADDED
	// expect_test["Removed"] = [ "@/dir_A/dir_B", "@/dir_A/dir_B/dir_C", "@/dir_A/dir_B/dir_C/file_1" ];
	expect_test["Added"] = [ "@/dir_A",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);


    }

}
