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
// Some basic tests for doing a MERGE when there is dirt in the WD.
//////////////////////////////////////////////////////////////////

function st_merge_dirty_WD()
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
        this.do_fsobj_mkdir("dir_0/dir_00");
        this.do_fsobj_create_file("file_0");
        this.do_fsobj_create_file("dir_0/file_00");
        this.do_fsobj_create_file("dir_0/dir_00/file_000");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 0");

        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_mkdir("dir_1/dir_11");
        this.do_fsobj_create_file("file_1");
        this.do_fsobj_create_file("dir_1/file_11");
        this.do_fsobj_create_file("dir_1/dir_11/file_111");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 1");

        this.do_fsobj_mkdir("dir_2");
        this.do_fsobj_mkdir("dir_2/dir_22");
        this.do_fsobj_create_file("file_2");
        this.do_fsobj_create_file("dir_2/file_22");
        this.do_fsobj_create_file("dir_2/dir_22/file_222");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 2");

	this.rev_trunk_head = this.csets[ this.csets.length - 1 ];

	// update back to '1' and start a new branch '3':
	//      0
	//     /
	//    1
	//   / \
	//  2   3

	this.do_update_when_clean_by_name("Simple ADDs 1");
        this.do_fsobj_mkdir("dir_3");
        this.do_fsobj_mkdir("dir_3/dir_33");
        this.do_fsobj_create_file("file_3");
        this.do_fsobj_create_file("dir_3/file_33");
        this.do_fsobj_create_file("dir_3/dir_33/file_333");
	vscript_test_wc__remove_file("dir_1/dir_11/file_111");
	vscript_test_wc__remove_dir("dir_1/dir_11");
        vscript_test_wc__addremove();
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit_in_branch("Simple ADDs 3");

	this.rev_branch_head = this.csets_in_branch[ this.csets_in_branch.length - 1 ];

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);
    }

    //////////////////////////////////////////////////////////////////
    //
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

    this.do_exec_merge = function(rev, bExpectFail, msgExpected)
    {
	print("vv merge --rev=" + rev);
	//var o = sg.exec(vv, "merge", "--rev="+rev, "--verbose");
	var o = sg.exec(vv, "merge", "--rev="+rev);
	
	print("STDOUT:\n" + o.stdout);
	print("STDERR:\n" + o.stderr);

	if (bExpectFail)
	{
	    testlib.ok( o.exit_status == 1, "Exit status should be 1" );
	    testlib.ok( o.stderr.search(msgExpected) >= 0, "Expecting error message [" + msgExpected + "]. Received:\n" + o.stderr);
	}
	else
	{
	    testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	}
    }

    this.do_exec_merge__allow_dirty = function(rev, bExpectFail, msgExpected)
    {
	print("vv merge --rev=" + rev + " --allow-dirty");
	//var o = sg.exec(vv, "merge", "--rev="+rev, "--verbose");
	var o = sg.exec(vv, "merge", "--rev="+rev, "--allow-dirty");
	
	print("STDOUT:\n" + o.stdout);
	print("STDERR:\n" + o.stderr);

	if (bExpectFail)
	{
	    testlib.ok( o.exit_status == 1, "Exit status should be 1" );
	    testlib.ok( o.stderr.search(msgExpected) >= 0, "Expecting error message [" + msgExpected + "]. Received:\n" + o.stderr);
	}
	else
	{
	    testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	}
    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.test_clean_1 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// make sure basic clean merge works as expected.

	this.do_update_by_rev(this.rev_trunk_head);
	this.do_exec_merge(this.rev_branch_head, 0, "");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_3/dir_33",
					 "@/file_3",
					 "@/dir_3/file_33", 
					 "@/dir_3/dir_33/file_333" ];
	expect_test["Removed"] = [ "@b/dir_1/dir_11",
				   "@b/dir_1/dir_11/file_111" ];
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.test_dirty_1 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// create some dirt in the WD and then try to merge
	//      0
	//     /
	//    1
	//   / \
	//  2+  3
	//   \ /
	//    M

	this.do_update_by_rev(this.rev_trunk_head);

	// create some FOUND files that DO NOT interfere.

	this.do_fsobj_create_file("dirt_file_a");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/dirt_file_a" ];
        vscript_test_wc__confirm_wii(expect_test);

	this.do_exec_merge(this.rev_branch_head, 0, "");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_3/dir_33",
					 "@/file_3",
					 "@/dir_3/file_33", 
					 "@/dir_3/dir_33/file_333" ];
	expect_test["Removed"] = [ "@b/dir_1/dir_11",
				   "@b/dir_1/dir_11/file_111" ];
	expect_test["Found"] = [ "@/dirt_file_a" ];
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.test_dirty_2 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// create some dirt in the WD and then try to merge
	//      0
	//     /
	//    1
	//   / \
	//  2+  3
	//   \ /
	//    M

	this.do_update_by_rev(this.rev_trunk_head);

	// create some FOUND files that DO NOT interfere.
	// IGNORED files should behave just like FOUND files.

	this.do_fsobj_create_file("dirt_file_a~");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/dirt_file_a~" ];
        vscript_test_wc__confirm_wii(expect_test);

	this.do_exec_merge(this.rev_branch_head, 0, "");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_3/dir_33",
					 "@/file_3",
					 "@/dir_3/file_33", 
					 "@/dir_3/dir_33/file_333" ];
	expect_test["Removed"] = [ "@b/dir_1/dir_11",
				   "@b/dir_1/dir_11/file_111" ];
	expect_test["Found"] = [ "@/dirt_file_a~" ];
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.test_dirty_3 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// create some dirt in the WD and then try to merge
	//      0
	//     /
	//    1
	//   / \
	//  2+  3
	//   \ /
	//    M

	this.do_update_by_rev(this.rev_trunk_head);

	// create some FOUND files that DO NOT interfere.
	// IGNORED files should behave just like FOUND files.

        this.do_fsobj_mkdir("dirt_dir_a");		// a plain FOUND
        this.do_fsobj_mkdir("dirt_dir_a/Debug");	// a plain FOUND (but also an IGNORABLE)
	this.do_fsobj_create_file("dirt_dir_a/Debug/dirt_file_a~");
	this.do_fsobj_create_file("dirt_dir_a/Debug/dirt_file_a");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/dirt_dir_a",
				 "@/dirt_dir_a/Debug",
				 "@/dirt_dir_a/Debug/dirt_file_a~",
				 "@/dirt_dir_a/Debug/dirt_file_a" ];
        vscript_test_wc__confirm_wii(expect_test);

	this.do_exec_merge(this.rev_branch_head, 0, "");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_3/dir_33",
					 "@/file_3",
					 "@/dir_3/file_33", 
					 "@/dir_3/dir_33/file_333" ];
	expect_test["Removed"] = [ "@b/dir_1/dir_11",
				   "@b/dir_1/dir_11/file_111" ];
	expect_test["Found"] = [ "@/dirt_dir_a",
				 "@/dirt_dir_a/Debug",
				 "@/dirt_dir_a/Debug/dirt_file_a~",
				 "@/dirt_dir_a/Debug/dirt_file_a" ];
        vscript_test_wc__confirm_wii(expect_test);
    }

    //////////////////////////////////////////////////////////////////

    this.test_interference_1 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// create some dirt in the WD and then try to merge
	//      0
	//     /
	//    1
	//   / \
	//  2+  3
	//   \ /
	//    M

	this.do_update_by_rev(this.rev_trunk_head);

	// create some FOUND files that WILL interfere.

	this.do_fsobj_create_file("file_3");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/file_3" ];
        vscript_test_wc__confirm_wii(expect_test);

	// MERGE will try to rename the FOUND file_3 because of the collision
	// to an unresolved temp name.  But this should fail because it is
	// uncontrolled.
	this.do_exec_merge(this.rev_branch_head, 1,
			   "Attempt to operate on an item which is not under version control");
    }

    this.test_interference_2 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// create some dirt in the WD and then try to merge
	//      0
	//     /
	//    1
	//   / \
	//  2+  3
	//   \ /
	//    M

	this.do_update_by_rev(this.rev_trunk_head);

	// create some FOUND files that WILL interfere.

	this.do_fsobj_create_file("FILE_3");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/FILE_3" ];
        vscript_test_wc__confirm_wii(expect_test);

	// portability conflict would cause rename of uncontrolled item.
	this.do_exec_merge(this.rev_branch_head, 1,
			   "Attempt to operate on an item which is not under version control");
    }

    this.test_interference_3 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// create some dirt in the WD and then try to merge
	//      0
	//     /
	//    1
	//   / \
	//  2+  3
	//   \ /
	//    M

	this.do_update_by_rev(this.rev_trunk_head);

	// create some FOUND files that WILL interfere.

	this.do_fsobj_create_file("dir_1/dir_11/other_111");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/dir_1/dir_11/other_111" ];
        vscript_test_wc__confirm_wii(expect_test);

	// parent directory deleted
	this.do_exec_merge(this.rev_branch_head, 1,
			   "Attempt to operate on an item which is not under version control");
    }

    //////////////////////////////////////////////////////////////////

    this.test_change_1 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge in the branch.
	// create some dirt in the WD and then try to merge
	//      0
	//     /
	//    1
	//   / \
	//  2+  3
	//   \ /
	//    M

	this.do_update_by_rev(this.rev_trunk_head);

	// change some files that are under version control.

	this.do_fsobj_append_file("file_2");

	var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_2" ];
        vscript_test_wc__confirm_wii(expect_test);

	// we now disallow dirty-merges unless "--allow-dirty" is set.
	this.do_exec_merge(this.rev_branch_head, 1, "This type of merge requires a clean working copy");

	// In WC version we allow dirty merges with modified files.
	this.do_exec_merge__allow_dirty(this.rev_branch_head, 0, "");
    }

}
