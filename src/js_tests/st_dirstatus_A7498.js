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

function st_dirstatus_A7498()
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

        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_mkdir("dir_1/dir_2");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4/dir_5");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7");
	this.do_fsobj_create_file("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file1.txt");
	this.do_fsobj_create_file("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file2.txt");
	this.do_fsobj_create_file("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file3.txt");
	this.do_fsobj_create_file("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file4.txt");
	this.do_fsobj_create_file("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file5.txt");
	this.do_fsobj_create_file("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file6.txt");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 1");
    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	//////////////////////////////////////////////////////////////////
	// start with clean status
        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	var ds = sg.wc.get_item_dirstatus_flags( { "src" : "dir_1" } );
	print(sg.to_json__pretty_print(ds));
	testlib.ok( (ds.changes == false) );
	testlib.ok( (ds.status.isDirectory == true) );

	//////////////////////////////////////////////////////////////////
	// make some deep down dirt

	vscript_test_wc__rename("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file1.txt", "file1_renamed.txt");

        var expect_test = new Array;
	expect_test["Renamed"] = [ "@/dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file1_renamed.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

	var ds = sg.wc.get_item_dirstatus_flags( { "src" : "dir_1" } );
	print(sg.to_json__pretty_print(ds));
	testlib.ok( (ds.changes == true) );
	testlib.ok( (ds.status.isDirectory == true) );
    }
}
