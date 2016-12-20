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

function st_ff_merge_01()
{
    var map = new Array();

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.my_do_commit = function(label)
    {
	this.do_commit(label);
	map[label] = sg.wc.parents()[0];
    }

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        //////////////////////////////////////////////////////////////////
        // fetch the HID of the initial commit.

        this.csets.push( sg.wc.parents()[0] );
        this.names.push( "*Initial Commit*" );

        //////////////////////////////////////////////////////////////////
	//
	//      0
	//      |
	//      1
	//      |
	//      2  # branch_a head
	//     /
	//    3
	//    |
	//    4
	//    |
	//    5    # branch_b head

	sg.exec(vv, "branch", "new", "branch_a");
        this.do_fsobj_mkdir("dir_0");
        this.do_fsobj_mkdir("dir_0/dir_00");
        this.do_fsobj_create_file("file_0");
        this.do_fsobj_create_file("dir_0/file_00");
        this.do_fsobj_create_file("dir_0/dir_00/file_000");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_0");

        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_mkdir("dir_1/dir_11");
        this.do_fsobj_create_file("file_1");
        this.do_fsobj_create_file("dir_1/file_11");
        this.do_fsobj_create_file("dir_1/dir_11/file_111");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_1");

        this.do_fsobj_mkdir("dir_2");
        this.do_fsobj_mkdir("dir_2/dir_22");
        this.do_fsobj_create_file("file_2");
        this.do_fsobj_create_file("dir_2/file_22");
        this.do_fsobj_create_file("dir_2/dir_22/file_222");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_2");

	sg.exec(vv, "branch", "new", "branch_b");

        this.do_fsobj_mkdir("dir_3");
        this.do_fsobj_mkdir("dir_3/dir_33");
        this.do_fsobj_create_file("file_3");
        this.do_fsobj_create_file("dir_3/file_33");
        this.do_fsobj_create_file("dir_3/dir_33/file_333");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_3");

        this.do_fsobj_mkdir("dir_4");
        this.do_fsobj_mkdir("dir_4/dir_44");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_4");

        this.do_fsobj_mkdir("dir_5");
        this.do_fsobj_mkdir("dir_5/dir_55");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_5");

	//////////////////////////////////////////////////////////////////

	print("");
	print("================================================================");
	var o = sg.exec(vv, "history");
	print("History:");
	print(o.stdout);
	print("");
	print("================================================================");
	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);
	print("");
	print("================================================================");
	print("");
    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.do_update_by_rev_attached = function(rev, attach)
    {
	print("vv update --rev=" + rev + " --attach=" + attach);
	var o = sg.exec(vv, "update", "--rev="+rev, "--attach="+attach);

	//print("STDOUT:\n" + o.stdout);
	//print("STDERR:\n" + o.stderr);

	testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	this.assert_current_baseline(rev);
    }

    this.do_update_by_branch = function(branch)
    {
	print("vv update --branch=" + branch);
	var o = sg.exec(vv, "update", "--branch", branch);
	testlib.ok( o.exit_status == 0, "Exit status should be 0" );

	var t = sg.exec(vv, "branch");
	testlib.ok( t.exit_status == 0, "Exit status should be 0" );
	testlib.ok( t.stdout.indexOf(branch) >= 0, "Expect implicit attach." );
    }

    this.do_exec_merge_by_rev = function(rev, bExpectNoOp, bExpectFail, msgExpected)
    {
	print("vv merge --rev=" + rev);
	var o = sg.exec(vv, "merge", "--rev="+rev, "--verbose");
	
	print("STDOUT:\n" + o.stdout);
	print("STDERR:\n" + o.stderr);

	if (bExpectNoOp)
	{
	    testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	    testlib.ok( o.stdout.search(msgExpected) >= 0, 
			"Expecting no-op message [" + msgExpected + "]. Received:\n" + o.stdout);
	}
	else if (bExpectFail)
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

	return o;
    }

    this.do_exec_merge_by_branch = function(branch, bExpectNoOp, bExpectFail, msgExpected)
    {
	print("vv merge --branch=" + branch);
	var o = sg.exec(vv, "merge", "--branch="+branch, "--verbose");
	
	print("STDOUT:\n" + o.stdout);
	print("STDERR:\n" + o.stderr);

	if (bExpectNoOp)
	{
	    testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	    testlib.ok( o.stdout.search(msgExpected) >= 0, 
			"Expecting no-op message [" + msgExpected + "]. Received:\n" + o.stdout);
	}
	else if (bExpectFail)
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

	return o;
    }

    this.do_exec_merge_by_branch__allow_dirty = function(branch, bExpectNoOp, bExpectFail, msgExpected)
    {
	print("vv merge --branch=" + branch + " --allow-dirty");
	var o = sg.exec(vv, "merge", "--branch="+branch, "--verbose", "--allow-dirty");
	
	print("STDOUT:\n" + o.stdout);
	print("STDERR:\n" + o.stderr);

	if (bExpectNoOp)
	{
	    testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	    testlib.ok( o.stdout.search(msgExpected) >= 0, 
			"Expecting no-op message [" + msgExpected + "]. Received:\n" + o.stdout);
	}
	else if (bExpectFail)
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

	return o;
    }

    this.do_exec_merge_by_branch__no_ff = function(branch, bExpectNoOp, bExpectFail, msgExpected)
    {
	print("vv merge --no-ff --branch=" + branch);
	var o = sg.exec(vv, "merge", "--no-ff", "--branch="+branch, "--verbose");
	
	print("STDOUT:\n" + o.stdout);
	print("STDERR:\n" + o.stderr);

	if (bExpectNoOp)
	{
	    testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	    testlib.ok( o.stdout.search(msgExpected) >= 0, 
			"Expecting no-op message [" + msgExpected + "]. Received:\n" + o.stdout);
	}
	else if (bExpectFail)
	{
	    testlib.ok( o.exit_status == 1, "Exit status should be 1" );
	    testlib.ok( o.stderr.search(msgExpected) >= 0,
			"Expecting error message [" + msgExpected + "]. Received:\n" + o.stderr);
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

	return o;
    }

    //////////////////////////////////////////////////////////////////

    this.test_simple_1 = function()
    {
	this.force_clean_WD();
	this.do_update_by_branch("branch_b");
	this.force_clean_WD();

	// try merge with self.  should fail w/ or w/o --no-ff
	this.do_exec_merge_by_branch__no_ff("branch_b", 1, 0, "The baseline already includes the merge target. No merge is needed.");
	this.do_exec_merge_by_branch(       "branch_b", 1, 0, "The baseline already includes the merge target. No merge is needed.");

	// try to merge with branch_a -- make sure that ff case rejects it too.
	this.do_exec_merge_by_branch__no_ff("branch_a", 1, 0, "The baseline already includes the merge target. No merge is needed.");
	this.do_exec_merge_by_branch(       "branch_a", 1, 0, "The baseline already includes the merge target. No merge is needed.");

	// update to non-head and try ff-merge
	this.do_update_by_rev_attached(map["cset_4"], "branch_b");
	this.do_exec_merge_by_branch(       "branch_a", 1, 0, "The baseline already includes the merge target. No merge is needed.");

	// detach WD and try ff-merge
	sg.exec(vv, "branch", "detach");
	this.do_exec_merge_by_branch(       "branch_a", 1, 0, "The baseline already includes the merge target. No merge is needed.");

	//////////////////////////////////////////////////////////////////
	// switch to branch_a and try to ff with branch_b.
	// this should UPDATE and MOVE-HEAD.

	this.do_update_by_branch("branch_a");
	this.force_clean_WD();

	this.do_exec_merge_by_branch__no_ff( "branch_b", 0, 1, "The merge target is a descendant of the baseline");
	var o = this.do_exec_merge_by_branch("branch_b", 0, 0, "");
	
	var parents_now = sg.wc.parents();
	testlib.ok( (parents_now.length == 1), "Expect 1 parent.");
	var parent_now = parents_now[0];
	testlib.ok( (parent_now == map["cset_5"]), "Expected update.");
	
	print("================================================================");
	print("Parents after ff-merge:");
	var o = sg.exec(vv, "parents");
	print(o.stdout);
	testlib.ok( (o.stdout.indexOf("branch:  branch_b") >= 0), "Expect branch_b");
	testlib.ok( (o.stdout.indexOf("branch:  branch_a") >= 0), "Expect branch_a");

	print("");
	print("================================================================");
	print("Heads after ff-merge:");
	var o = sg.exec(vv, "heads");
	print(o.stdout);

	//////////////////////////////////////////////////////////////////
	// See if ff-merge respects --allow-dirty
	// See W3173, W3175, W7925.

	var o = sg.exec(vv, "branch", "add-head", "branch_z", "-r", map["cset_1"]);
	print(o.stdout);
	var o = sg.exec(vv, "branch", "add-head", "branch_y", "-r", map["cset_3"]);
	print(o.stdout);
	this.do_update_by_branch("branch_z");
	
	vscript_test_wc__rename("dir_0", "dir_0_renamed");

	this.do_exec_merge_by_branch__no_ff( "branch_y", 0, 1, "The merge target is a descendant of the baseline");
	var o = this.do_exec_merge_by_branch("branch_y", 0, 1, "This type of merge requires a clean working copy");
	var o = this.do_exec_merge_by_branch__allow_dirty("branch_y", 0, 0, "");
	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/dir_0_renamed" ];
        vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__revert_all();

	//////////////////////////////////////////////////////////////////
	// See P3129 -- verify that ff-merge complains if no whoami is set.
	//
	//    |
	//    5    # branch_b head
	//    |
	//    6    # branch_a head

	this.do_update_by_branch("branch_a");
	this.force_clean_WD();
	this.do_fsobj_create_file("file_6");
	vscript_test_wc__addremove();
	this.my_do_commit("cset_6");

	this.do_update_by_branch("branch_b");

	print("reset /machine username");
	o = sg.exec(vv, "config", "reset", "whoami/username");
	o = sg.exec(vv, "config", "reset", "whoami/userid");
	var repo = sg.open_local_repo();
	var admin = repo.admin_id;
	repo.close();
	print("reset /admin userid");
	o = sg.exec(vv, "config", "reset", "/admin/"+admin+"/whoami/userid");
	print("whoami is now:");
	o = sg.exec(vv, "whoami");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_status == 1), "Expect whoami to fail now.");
	testlib.ok( (o.stderr.indexOf("No current user") >= 0), "Expect whoami to fail now.");
	print("config is now:");
	o = sg.exec(vv, "config");
	print(o.stdout);
	print(o.stderr);

	this.do_exec_merge_by_branch(       "branch_a", 0, 1, "No current user is set");

    }

}
