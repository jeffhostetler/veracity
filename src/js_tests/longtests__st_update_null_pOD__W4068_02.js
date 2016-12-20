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
// Some basic tests for doing arbitrary (unrelated) updates when
// there is ignorable dirt.
//////////////////////////////////////////////////////////////////

function st_update_null_pOD__W4068_02()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        //////////////////////////////////////////////////////////////////
        // fetch the HID of the initial commit.

        this.csets.push( sg.wc.parents()[0] );
        this.names.push( "*Initial Commit*" );

        //////////////////////////////////////////////////////////////////
	// create 3 csets:
	//
	//      0
	//     /
	//    1
	//   /
	//  2
	//

	this.do_fsobj_create_file_exactly(".sgignores", ("ignored_dir\n"
							 + "*.foo\n")  );
        this.do_fsobj_mkdir("dir_0");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 0");

        this.do_fsobj_mkdir("dir_1");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 1");

        this.do_fsobj_mkdir("dir_2");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 2");

    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.do_fsobj_mkdir("dir_1/dir_x");
	this.do_fsobj_mkdir("dir_1/dir_x/dir_y");
	this.do_fsobj_mkdir("dir_1/dir_x/dir_y/dir_z");
	this.do_fsobj_mkdir("dir_1/dir_x/dir_y/dir_z/found_dir");
	this.do_fsobj_create_file_exactly("dir_1/dir_x/dir_y/dir_z/found_dir/file1.txt",
					 "Hello World!\n");

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("Confirm that everything is present.");

        var expect_test = new Array;
        expect_test["Found"] = [ "@/dir_1/dir_x",
				 "@/dir_1/dir_x/dir_y",
				 "@/dir_1/dir_x/dir_y/dir_z",
				 "@/dir_1/dir_x/dir_y/dir_z/found_dir",
				 "@/dir_1/dir_x/dir_y/dir_z/found_dir/file1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("Confirm that .sgignores is respected.");

        var expect_test = new Array;
        expect_test["Found"] = [ "@/dir_1/dir_x",
				 "@/dir_1/dir_x/dir_y",
				 "@/dir_1/dir_x/dir_y/dir_z",
				 "@/dir_1/dir_x/dir_y/dir_z/found_dir",
				 "@/dir_1/dir_x/dir_y/dir_z/found_dir/file1.txt" ];
        vscript_test_wc__confirm(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("Try to do UPDATE with dirt present.");

	this.do_update_when_dirty_by_name("Simple ADDs 1");

        var expect_test = new Array;
        expect_test["Found"] = [ "@/dir_1/dir_x",
				 "@/dir_1/dir_x/dir_y",
				 "@/dir_1/dir_x/dir_y/dir_z",
				 "@/dir_1/dir_x/dir_y/dir_z/found_dir",
				 "@/dir_1/dir_x/dir_y/dir_z/found_dir/file1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

    }
}
