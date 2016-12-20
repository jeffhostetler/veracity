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

function st_move__W9293()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

        this.do_fsobj_mkdir("dir1");
        this.do_fsobj_mkdir("dir1/dir2");
        this.do_fsobj_mkdir("dir1/dir2/dir3");
        this.do_fsobj_create_file("file1");
        vscript_test_wc__addremove();
        this.do_fsobj_mkdir("dir1/dir2/uncontrolled");

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir1",
				 "@/dir1/dir2",
				 "@/dir1/dir2/dir3",
				 "@/file1" ];
	expect_test["Found"] = [ "@/dir1/dir2/uncontrolled" ];
        vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__commit("Added stuff");

	vscript_test_wc__remove_dir(  "dir1/dir2/dir3" );

        var expect_test = new Array;
	expect_test["Removed"] = [ "@b/dir1/dir2/dir3" ];
	expect_test["Found"]   = [ "@/dir1/dir2/uncontrolled" ];
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Try to move into bogus directory");
	print("");

	vscript_test_wc__move__expect_error( "file1", "dir1/dir2/bogus",
					     "Destination directory not found");

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Try to move into deleted directory");
	print("");

	vscript_test_wc__move__expect_error( "file1", "@b/dir1/dir2/dir3",
					     "Destination directory has pending delete");

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Try to move into uncontrolled directory");
	print("");

	vscript_test_wc__move__expect_error( "file1", "@/dir1/dir2/uncontrolled",
					     "Destination directory not under version control");

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Verify nothing changed");
	print("");

	// nothing should have changed.
        vscript_test_wc__confirm_wii(expect_test);

    }
}
