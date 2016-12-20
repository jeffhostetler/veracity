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
// REVERT tests -- add (but do not commit) something and then revert
// it and confirm that the item remains in a FOUND state.
//////////////////////////////////////////////////////////////////

function st_revert__unadd_01()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

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
	print("Starting REVERT --all");
	print("");

	vscript_test_wc__revert_all( "unadd_01" );

        var expect_test = new Array;
	expect_test["Found"] = [ "@/dir_A",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Re-adding items...");
	print("");

        vscript_test_wc__addremove();

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Starting REVERT dir_A by name");
	print("");

	vscript_test_wc__revert_items( "dir_A" );

        var expect_test = new Array;
	expect_test["Found"] = [ "@/dir_A" ];
	expect_test["Added"] = [ "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Re-adding items...");
	print("");

        vscript_test_wc__addremove();

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A",
				 "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Starting REVERT file_0 by name");
	print("");

	vscript_test_wc__revert_items( "file_0" );

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir_A" ];
	expect_test["Found"] = [ "@/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

    }
}
