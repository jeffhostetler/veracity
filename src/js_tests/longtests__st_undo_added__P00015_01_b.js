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

function st_undo_added__P00015_01_b()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

        this.do_fsobj_create_file("baseFile");
	vscript_test_wc__addremove();
	this.do_commit("add baseFile");
	var cset_base = sg.wc.parents()[0];

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Add items...");
	print("");

        this.do_fsobj_mkdir("dir_A");
        this.do_fsobj_create_file("file_0");
        this.do_fsobj_create_file("dir_A/file_A");
        vscript_test_wc__addremove();

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/dir_A/file_A",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Move a COMMITTED file into ADDED directory...");
	print("");

	// move committed baseFile into dir_A.
	// this should cause ADD+REMOVE-style delete of dir_A to fail.

	vscript_test_wc__move("baseFile", "dir_A");

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/dir_A/file_A",
				 "@/file_0" ];
	expect_test["Moved"] = [ "@/dir_A/baseFile" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Use REMOVE to try to 'unadd' them...");
	print("");

	var result = sg.exec(vv, "remove", "dir_A");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status != 0), "'vv remove' should fail" );
	testlib.ok( (result.stderr.indexOf("The object cannot be safely removed") != -1), "Expect failure" );

	// and nothing should have changed in the WD.

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/dir_A/file_A",
				 "@/file_0" ];
	expect_test["Moved"] = [ "@/dir_A/baseFile" ];
        vscript_test_wc__confirm_wii(expect_test);

    }

}
