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
// Generate a merge conflict.
//////////////////////////////////////////////////////////////////

function st_merge__Y7799()
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
	//     /
	//    1
	
        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_mkdir("dir_2");
        this.do_fsobj_create_file("dir_1/common.txt");
        this.do_fsobj_create_file("dir_2/common.txt");
        vscript_test_wc__addremove();
	this.gid_1 = sg.wc.get_item_gid_path({"src":"dir_1/common.txt"});
	this.gid_2 = sg.wc.get_item_gid_path({"src":"dir_2/common.txt"});
        this.do_commit("Simple ADDs");

        vscript_test_wc__move("dir_1/common.txt", ".");
        this.do_commit("Move 1");

	this.rev_trunk_head = this.csets[ this.csets.length - 1 ];

	//////////////////////////////////////////////////////////////////
	// update back to '0' and start a new branch '2':
	//      0
	//     / \
	//    1   2
	//
	// this will generate a hard collision.

	this.do_update_when_clean_by_name("Simple ADDs");
	sg.exec(vv, "branch", "attach", "master");

        vscript_test_wc__move("dir_2/common.txt", ".");
        this.do_commit_in_branch("Move 2");

	this.rev_branch_head = this.csets_in_branch[ this.csets_in_branch.length - 1 ];

	//////////////////////////////////////////////////////////////////

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);

	print("");
	print("rev_trunk_head  := " + this.rev_trunk_head);
	print("rev_branch_head := " + this.rev_branch_head);
	print("");
	print("================================================================");
	print("End of Setup");
	print("================================================================");
	print("");
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
	print("");
	print("================================================================");
	print("Begin test");
	print("================================================================");
	print("");

	this.force_clean_WD();

	// set WD to the trunk.
	// merge (fold) in the branch.

	this.do_update_by_rev(this.rev_trunk_head);
	this.do_exec_merge(this.rev_branch_head, 0, "");

        var expect_test = new Array;
	expect_test["Renamed"] = [ this.gid_1,
				   this.gid_2 ];
	expect_test["Moved"]   = [ this.gid_2 ];
	expect_test["Unresolved"] = [ this.gid_1,
				      this.gid_2 ];
	expect_test["Choice Unresolved (Location)"] = [ this.gid_1,
							this.gid_2 ];
        vscript_test_wc__confirm_wii(expect_test);
    }

}
