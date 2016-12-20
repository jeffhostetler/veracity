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

function st_revert__W9231()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

        this.do_fsobj_mkdir("dir1");
        this.do_fsobj_mkdir("dir1/dir2");
        this.do_fsobj_mkdir("dir1/dir3");
        this.do_fsobj_create_file("dir1/dir2/file1");
        vscript_test_wc__addremove();

        var expect_test = new Array;
	expect_test["Added"] = [ "@/dir1",
				 "@/dir1/dir2",
				 "@/dir1/dir3",
				 "@/dir1/dir2/file1" ];
        vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__commit("Added stuff");

	vscript_test_wc__move(   "dir1/dir2/file1", "dir1/dir3" );
	vscript_test_wc__rename( "dir1/dir2", "dir2a" );
	vscript_test_wc__rename( "dir1/dir3", "dir3a" );

        var expect_test = new Array;
	expect_test["Moved"]   = [ "@/dir1/dir3a/file1" ];
	expect_test["Renamed"] = [ "@/dir1/dir2a",
				   "@/dir1/dir3a" ];
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Starting REVERT-ITEMS on 'file1'...");
	print("");

	// this should only un-move it; nothing should happen to the parent directories.
	vscript_test_wc__revert_items( [ "@/dir1/dir3a/file1" ] );

        var expect_test = new Array;
	expect_test["Renamed"] = [ "@/dir1/dir2a",
				   "@/dir1/dir3a" ];
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Move it again and try a non-recursive revert for just the directory rename.")
	print("");

	vscript_test_wc__move(   "dir1/dir2a/file1", "dir1/dir3a" );

        var expect_test = new Array;
	expect_test["Moved"]   = [ "@/dir1/dir3a/file1" ];
	expect_test["Renamed"] = [ "@/dir1/dir2a",
				   "@/dir1/dir3a" ];
        vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__revert_items__np( { "src"     : [ "@/dir1/dir3a" ],
					     "depth"   : 0,
					     "verbose" : true } );

        var expect_test = new Array;
	expect_test["Moved"]   = [ "@/dir1/dir3/file1" ];
	expect_test["Renamed"] = [ "@/dir1/dir2a" ];
        vscript_test_wc__confirm_wii(expect_test);

    }
}
