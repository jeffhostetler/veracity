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
// Tests REMVOE of already deleted item.
//////////////////////////////////////////////////////////////////

function st_remove_deleted__R0621_02()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

	this.do_fsobj_mkdir("dir_A");
	this.do_fsobj_mkdir("dir_A/dir_B");
	this.do_fsobj_mkdir("dir_A/dir_B/dir_C");
        this.do_fsobj_create_file("dir_A/dir_B/dir_C/file_1");
	vscript_test_wc__addremove();
	this.do_commit("add initial files");
	var cset_base = sg.wc.parents()[0];

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
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Verify that normal 'vv remove' converts LOST to REMOVED...");
	print("");

	var result = sg.exec(vv, "remove", "dir_A/dir_B/dir_C");
	if (result.exit_status != 0)
	    print(dumpObj(result, "sg.exec()", "", 0));

	testlib.ok( (result.exit_status == 0), "'vv remove dir_A/dir_B/dir_C' should pass" );

        var expect_test = new Array;
	expect_test["Lost"] = [ "@/dir_A/dir_B" ];
	expect_test["Removed"] = [ "@b/dir_A/dir_B/dir_C",
				   "@b/dir_A/dir_B/dir_C/file_1"
				 ];
        vscript_test_wc__confirm_wii(expect_test);

    }

}
