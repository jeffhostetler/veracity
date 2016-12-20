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
// Test for sprawl-912.
// Create a merge with a delete-vs-edit file conflict (where the
// delete is overruled).
// Try to force the delete (without using RESOLVE) so that you can
// commit without the file.  (Previously, you'd have to commit with
// the file and then delete it in a subsequent commit.)
//////////////////////////////////////////////////////////////////

function st_merge_delete_vs_edit__SPRAWL_912()
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
	// create:
	//
	//      0
	//      |
	//      1      create a file in (1)
	//      |
	//      2
	//     / \
	//    3   4    edit the file in (3); delete the file in (4)

        this.do_fsobj_mkdir("dir_0");
        vscript_test_wc__addremove();
        this.do_commit("CSET 0");

        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_create_file_exactly("dir_1/file_11", "Version 1");
        vscript_test_wc__addremove();
        this.do_commit("CSET 1");

        this.do_fsobj_mkdir("dir_2");
        vscript_test_wc__addremove();
        this.do_commit("CSET 2");

        this.do_fsobj_mkdir("dir_3");
        vscript_test_wc__addremove();
	this.do_fsobj_append_file("dir_1/file_11", "Hello from [3].");
        this.do_commit("CSET 3");

	this.rev_trunk_head = this.csets[ this.csets.length - 1 ];

	//////////////////////////////////////////////////////////////////
	// update back to '2' and start the other branch.

	this.do_update_when_clean_by_name("CSET 2");
        this.do_fsobj_mkdir("dir_4");
	vscript_test_wc__remove_file("dir_1/file_11");
        vscript_test_wc__addremove();
	sg.exec(vv, "branch", "new", "newbranch");
        this.do_commit_in_branch("CSET 4");

	this.rev_branch_head = this.csets_in_branch[ this.csets_in_branch.length - 1 ];

	//////////////////////////////////////////////////////////////////

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
	var o = sg.exec(vv, "merge", "--rev="+rev, "--verbose");
	
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
    // NOTE: this.confirm() and vscript_test_wc__confirm_wii()
    //       compute a STATUS and compare that with the expected results.
    //       A STATUS reflects the changes made to the WD from the
    //       BASELINE.  So the STATUS should show how the FINAL RESULT
    //       changed from the BASELINE.
    //
    //       This feels a little odd because we are accustomed to
    //       think about the total net changes from the ANCESTOR to
    //       the FINAL RESULT.
    //
    //       So, for example, if a file was renamed between the
    //       ancestor and the baseline branch (but not in the other
    //       changeset), it will not show up in the status.  It will
    //       have the new name in the final result, but that is not
    //       a change relative to the baseline.
    //////////////////////////////////////////////////////////////////

    this.test_simple_tb_1 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge (fold) in the branch.
	// make sure basic clean merge works as expected.

	this.do_update_by_rev(this.rev_trunk_head);
	this.do_exec_merge(this.rev_branch_head, 0, "");

	// dir_1/file_11 is still in trunk with a delete-vs-edit conflict.

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_4" ];
	expect_test["Unresolved"]    = [ "@/dir_1/file_11" ];
	expect_test["Choice Unresolved (Existence)"]    = [ "@/dir_1/file_11" ];
        vscript_test_wc__confirm_wii(expect_test);

	// try to let the DELETE win.

	var result = sg.exec(vv, "remove", "dir_1/file_11");
	if (result.exit_status != 0)
	    print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "Remove should pass");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_4" ];
	expect_test["Removed"]       = [ "@b/dir_1/file_11" ];
	expect_test["Resolved"]      = [ "@b/dir_1/file_11" ];
	expect_test["Choice Resolved (Existence)"]      = [ "@b/dir_1/file_11" ];
        vscript_test_wc__confirm_wii(expect_test);

	var result = sg.exec(vv, "status", "--no-ignores");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "STATUS should pass");
    }

    this.test_simple_bt_1 = function()
    {
	this.force_clean_WD();

	// set WD to the branch.
	// merge (fold) in the trunk.
	// make sure basic clean merge works as expected.

	this.do_update_by_rev(this.rev_branch_head);
	this.do_exec_merge(this.rev_trunk_head, 0, "");

	// dir_1/file_11 was deleted from branch.  it should appear
	// as added because of the delete-vs-edit conflict.  that is,
	// it appears as added relative to the branch-baseline.

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_1/file_11" ];
        expect_test["Unresolved"]    = [ "@/dir_1/file_11" ];
        expect_test["Choice Unresolved (Existence)"]    = [ "@/dir_1/file_11" ];
        vscript_test_wc__confirm_wii(expect_test);

	// try to let the DELETE win.

	var result = sg.exec(vv, "remove", "dir_1/file_11");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "Remove should pass");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@c/dir_1/file_11" ];
	expect_test["Removed"]       = [ "@c/dir_1/file_11" ];
	expect_test["Resolved"]      = [ "@c/dir_1/file_11" ];
	expect_test["Choice Resolved (Existence)"]      = [ "@c/dir_1/file_11" ];
        vscript_test_wc__confirm_wii(expect_test);

	var result = sg.exec(vv, "status", "--no-ignores");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "STATUS should pass");

    }

}
