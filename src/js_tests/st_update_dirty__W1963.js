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
// there is ignorable dirt in the working directory.
//////////////////////////////////////////////////////////////////

function st_upate_dirty__W1963()
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

        this.do_fsobj_mkdir("dir_0");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 0");

        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_mkdir("dir_1/dir_2");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4/dir_5");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7");
	this.do_fsobj_create_file("dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/file.txt");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 1");

        this.do_fsobj_mkdir("dir_2");		// noise to get a new cset
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 2");

	// these should be equivalent
	// this.rev_trunk_head = this.csets[ this.csets.length - 1 ];
	this.rev_trunk_head = sg.wc.parents()[0];

	// update back to '1' and start a new branch '3':
	//      0
	//     /
	//    1
	//   / \
	//  2   3
	//       \
	//        4

	this.do_update_when_clean_by_name("Simple ADDs 1");
	var r = sg.exec(vv, "branch", "attach", "master");
	testlib.ok( (r.exit_status == 0), "vv branch attach master");
	if (r.exit_status != 0)
	    print(dumpObj(r,"vv branch attach master output", "", 0));

        this.do_fsobj_mkdir("dir_3");		// noise to get a new cset
        vscript_test_wc__addremove();
        this.do_commit_in_branch("Simple ADDs 3");

	vscript_test_wc__move("dir_1/dir_2/dir_3/dir_4", "dir_1");	// move dir_4 2 levels up
	vscript_test_wc__addremove();
	this.do_commit_in_branch("Simple ADDs 4");

	// these should be equivalent
	// this.rev_branch_head = this.csets_in_branch[ this.csets_in_branch.length - 1 ];
	this.rev_branch_head = sg.wc.parents()[0];

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);
    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	// we are looking at [4] the value in this.rev_branch_head.
	// verify that we can update from [4 ==> 2] and [2 ==> 4].
	// these are unrelated peers (non ancestor/descendant).

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("Verify that we can do CLEAN UPDATE [4 ==> 2]");

	vscript_test_wc__update( this.rev_trunk_head );
	vscript_test_wc__verifyCleanStatusWhenIgnoringIgnores();

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("Verify that we can do CLEAN UPDATE [2 ==> 4]");

	vscript_test_wc__update( this.rev_branch_head );
	vscript_test_wc__verifyCleanStatusWhenIgnoringIgnores();

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("Create an UNCONTROLLED file in the moved directory.");

	this.do_fsobj_create_file("dir_1/dir_4/dir_5/dir_6/dir_7/uncontrolled.txt");

        var expect_test_4d = new Array;
	expect_test_4d["Found"] = [ "@/dir_1/dir_4/dir_5/dir_6/dir_7/uncontrolled.txt" ];
        vscript_test_wc__confirm_wii(expect_test_4d);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("Verify that we can do DIRTY UPDATE [4 ==> 2]");

	vscript_test_wc__update( this.rev_trunk_head );

        var expect_test_42 = new Array;
	expect_test_42["Found"] = [ "@/dir_1/dir_2/dir_3/dir_4/dir_5/dir_6/dir_7/uncontrolled.txt" ];
        vscript_test_wc__confirm_wii(expect_test_42);
    }
}
