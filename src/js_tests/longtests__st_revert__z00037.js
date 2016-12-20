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
// REVERT tests to test bug Z00037.
//////////////////////////////////////////////////////////////////

function st_revert__z00037()
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

        this.do_fsobj_mkdir("dir_A");
        this.do_fsobj_mkdir("dir_A/dir_1");
        this.do_fsobj_mkdir("dir_B");
        this.do_fsobj_mkdir("dir_B/dir_2");
        this.do_fsobj_create_file("dir_A/dir_1/file_0");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs");

	this.rev_trunk_head = this.csets[ this.csets.length - 1 ];

	//////////////////////////////////////////////////////////////////

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);
    }

    //////////////////////////////////////////////////////////////////

    this.do_update_by_rev = function(rev)
    {
	print("vv update --rev=" + rev);
	var o = sg.exec(vv, "update", "--rev="+rev);

	//print("STDOUT:\n" + o.stdout);
	//print("STDERR:\n" + o.stderr);

	testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	this.assert_current_baseline(rev);
    }

    //////////////////////////////////////////////////////////////////

    this.test_simple_tb_1 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.

	this.do_update_by_rev(this.rev_trunk_head);

	// make some dirt:

	this.do_fsobj_append_file("dir_A/dir_1/file_0", "HELLO THERE.");
	vscript_test_wc__move("dir_A", "dir_B");

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/dir_B/dir_A/dir_1/file_0" ];
	expect_test["Moved"]    = [ "@/dir_B/dir_A" ];
        vscript_test_wc__confirm_wii(expect_test);

	var backup_file = vscript_test_wc__backup_name_of_file( "dir_B/dir_A/dir_1/file_0", "sg00" );

	// revert it all and make sure we don't crash or throw

	vscript_test_wc__revert_all();

        var expect_test = new Array;
	expect_test["Found"] = [ "@/" + backup_file ];
        vscript_test_wc__confirm_wii(expect_test);
    }
}
