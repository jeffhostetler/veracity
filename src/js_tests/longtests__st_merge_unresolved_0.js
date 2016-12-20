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
// Some MERGE tests to try to drive coverage so that we can see
// the output produced by the --verbose option and list the resolved
// and unresolved issues.
//////////////////////////////////////////////////////////////////

function st_merge_unresolved_0()
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
	vscript_test_wc__rename("dir_0/dir_00/file_000", "file_000_trunk");
	vscript_test_wc__rename("dir_0", "dir_0_trunk");
        this.do_commit("Simple ADDs 2");

	this.rev_trunk_head = this.csets[ this.csets.length - 1 ];

	//////////////////////////////////////////////////////////////////
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
	vscript_test_wc__rename("dir_0/dir_00/file_000", "file_000_branch");
	vscript_test_wc__rename("dir_0", "dir_0_branch");
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit_in_branch("Simple ADDs 3");

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

	print("vv status --no-ignores");
	var s = sg.exec(vv, "status", "--no-ignores");

	print("STDOUT:\n" + s.stdout);
	print("STDERR:\n" + s.stderr);

	testlib.ok( s.exit_status == 0, "Exit status should be 0" );

	print("vv resolve list --all");
	var r = sg.exec(vv, "resolve", "list", "--all");

	print("STDOUT:\n" + r.stdout);
	print("STDERR:\n" + r.stderr);

	testlib.ok( r.exit_status == 0, "Exit status should be 0" );
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

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_3/dir_33",
					 "@/file_3",
					 "@/dir_3/file_33", 
					 "@/dir_3/dir_33/file_333" ];
	expect_test["Removed"] = [ "@b/dir_1/dir_11",
				   "@b/dir_1/dir_11/file_111" ];
	expect_test["Renamed"] = [ "@/dir_0_trunk~g[0-9a-f]*/dir_00/file_000_trunk~g[0-9a-f]*",
				   "@/dir_0_trunk~g[0-9a-f]*" ];
	expect_test["Unresolved"] = [ "@/dir_0_trunk~g[0-9a-f]*/dir_00/file_000_trunk~g[0-9a-f]*",
				      "@/dir_0_trunk~g[0-9a-f]*" ];
	expect_test["Choice Unresolved (Name)"] = [ "@/dir_0_trunk~g[0-9a-f]*/dir_00/file_000_trunk~g[0-9a-f]*",
						    "@/dir_0_trunk~g[0-9a-f]*" ];
        vscript_test_wc__confirm_wii(expect_test);
    }

    this.test_simple_bt_1 = function()
    {
	this.force_clean_WD();

	// set WD to the branch.
	// merge (fold) in the trunk.
	// make sure basic clean merge works as expected.

	this.do_update_by_rev(this.rev_branch_head);
	this.do_exec_merge(this.rev_trunk_head, 0, "");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_2",
				 "@/dir_2/dir_22",
				 "@/file_2",
				 "@/dir_2/file_22", 
				 "@/dir_2/dir_22/file_222" ];
	expect_test["Renamed"] = [ "@/dir_0_branch~g[0-9a-f]*/dir_00/file_000_branch~g[0-9a-f]*",
				   "@/dir_0_branch~g[0-9a-f]*" ];
	expect_test["Unresolved"] = [ "@/dir_0_branch~g[0-9a-f]*/dir_00/file_000_branch~g[0-9a-f]*",
				      "@/dir_0_branch~g[0-9a-f]*" ];
	expect_test["Choice Unresolved (Name)"] = [ "@/dir_0_branch~g[0-9a-f]*/dir_00/file_000_branch~g[0-9a-f]*",
						    "@/dir_0_branch~g[0-9a-f]*" ];
        vscript_test_wc__confirm_wii(expect_test);
    }

}
