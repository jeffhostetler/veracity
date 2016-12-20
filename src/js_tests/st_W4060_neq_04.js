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

function st_W4060_neq_03()
{
    var map = new Array();
    var txt = new Array();

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.my_do_commit = function(label)
    {
	this.do_commit(label);
	map[label] = sg.wc.parents()[0];
	sg.exec(vv, "tag", "add", label);
	if (sg.fs.exists("file_0"))
	    txt[label] = sg.file.read("file_0");
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
	//                0
	//                |
	//                0a
	//                |
	//                0b
	//               / \
	//             0c   0d
	//               \ /
	//                0e
	//                |
	//                0f
	//               / \
	//              1   2
	//             / \ / \ 
	//            3   4   5
	//            |   |   |
	//            |   4a  |
	//             \ / \ /
	//             s0   s1
	//              |\ /|
	//             x0 X x1
	//              |/ \|
	//              6   7
	//
	// [] start with a file that exists in 0, 1, and 2.
	// [] modify the file in 0a...0f.
	// [] modify the file in 3 and 5.
	// [] delete the file during the merge creating 4.
	// [] accept the modified (rather than deleted) version
	//    of the file when creating merges s0 and s1.
	// [] s0 and s1 should be SPCAs.
	// [] noise nodes x0 and x1 to help keep the graph straight.
	// [] accept s0's version in 6 and s1's version in 7.
	//
	// When we try to merge 6 and 7, we consider this
	// portion of the DAG.
	//
	//                4a
	//               / \
	//              .   .
	//              .   .
	//              .   .
	//              6   7
	//               \ /
	//                8
	//
	// The file exists in both 6 and 7 (with the same GID),
	// but it does not exist in 4a.  So there is no ancestor
	// version available.  This causes the MERGE code to 
	// throw with a duplicate (GID) key because it looks 
	// like BOTH sides ADDED it.
	//

        this.do_fsobj_create_file("file_0");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_0");

	this.do_fsobj_append_file("file_0", "0a\n");
        this.my_do_commit("cset_0a");
	this.do_fsobj_append_file("file_0", "0b\n");
        this.my_do_commit("cset_0b");
	this.do_fsobj_append_file("file_0", "0c\n");
        this.my_do_commit("cset_0c");
	this.do_update_by_rev_attached(map["cset_0b"], "master");
	this.do_fsobj_append_file("file_0", "0d\n");
        this.my_do_commit("cset_0d");
	this.do_exec_merge(map["cset_0c"], 0, "");
	sg.exec(vv, "resolve", "accept", "other", "--all");
        this.my_do_commit("cset_0e");
	this.do_fsobj_append_file("file_0", "0f\n");
        this.my_do_commit("cset_0f");

        this.do_fsobj_create_file("file_1");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_1");

        this.do_fsobj_create_file("file_3");
        vscript_test_wc__addremove();
	this.do_fsobj_append_file("file_0", "THIS MESSAGE IS DIFFERENT IN 3 THAN IN 5.\n");
        this.my_do_commit("cset_3");

	this.do_update_by_rev_attached(map["cset_0f"], "master");

        this.do_fsobj_create_file("file_2");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_2");

        this.do_fsobj_create_file("file_5");
        vscript_test_wc__addremove();
	this.do_fsobj_append_file("file_0", "NOT THE SAME MESSAGE IN 3 AS IN 5.\n");
        this.my_do_commit("cset_5");

	this.do_update_by_rev_attached(map["cset_1"], "master");

	this.do_exec_merge(map["cset_2"], 0, "");
	vscript_test_wc__remove_file("file_0");
	this.my_do_commit("cset_4");

        this.do_fsobj_create_file("file_4a");
        vscript_test_wc__addremove();
	this.my_do_commit("cset_4a");

	this.do_exec_merge(map["cset_3"], 0, "");
	sg.exec(vv, "resolve", "accept", "other", "--all");
	this.my_do_commit("cset_s0");

	this.do_fsobj_create_file("file_x0");
        vscript_test_wc__addremove();
	this.my_do_commit("cset_x0");

	this.do_update_by_rev_attached(map["cset_4a"], "master");

	this.do_exec_merge(map["cset_5"], 0, "");
	sg.exec(vv, "resolve", "accept", "other", "--all");
	this.my_do_commit("cset_s1");

	this.do_fsobj_create_file("file_x1");
        vscript_test_wc__addremove();
	this.my_do_commit("cset_x1");

	this.do_exec_merge(map["cset_s0"], 0, "");
	sg.exec(vv, "resolve", "accept", "baseline", "--all");
	this.my_do_commit("cset_7");

	this.do_update_by_rev_attached(map["cset_x0"], "master");

	this.do_exec_merge(map["cset_s1"], 0, "");
	sg.exec(vv, "resolve", "accept", "baseline", "--all");
	this.my_do_commit("cset_6");

	// set WD to 7 after everything is constructed so that
	// we are consistent with the other tests.
	this.do_update_by_rev_attached(map["cset_7"], "master");

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);

	print("Tags are:");
	for (var m in map)
	    print(m + " " + map[m]);

    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.do_update_by_rev_attached = function(rev, branch)
    {
	print("vv update --rev=" + rev + " --attach=" + branch);
	var o = sg.exec(vv, "update", "--rev="+rev, "--attach="+branch);

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

	print("vv status --no-ignores --verbose");
	var s = sg.exec(vv, "status", "--no-ignores", "--verbose");
	print("STDOUT:\n" + s.stdout);
	print("STDERR:\n" + s.stderr);
	testlib.ok( s.exit_status == 0, "Exit status should be 0" );

	print("vv mstatus --no-ignores --verbose");
	var s = sg.exec(vv, "mstatus", "--no-ignores", "--verbose");
	print("STDOUT:\n" + s.stdout);
	print("STDERR:\n" + s.stderr);
	testlib.ok( s.exit_status == 0, "Exit status should be 0" );

	print("vv resolve list --all --verbose");
	var r = sg.exec(vv, "resolve", "list", "--all", "--verbose");
	print("STDOUT:\n" + r.stdout);
	print("STDERR:\n" + r.stderr);
	testlib.ok( r.exit_status == 0, "Exit status should be 0" );
    }

    //////////////////////////////////////////////////////////////////

    this.test_simple_1 = function()
    {
	var o = sg.exec(vv, "dump_lca", "-r", map["cset_6"], "-r", map["cset_7"], "--list");
	print("daglca(6,7):");
	print(o.stdout);

	testlib.ok( (o.stdout.indexOf( map["cset_4a"] + " LCA"  ) >= 0), "Expect 4a to be LCA.");
	testlib.ok( (o.stdout.indexOf( map["cset_s0"] + " SPCA" ) >= 0), "Expect s0 to be SPCA.");
	testlib.ok( (o.stdout.indexOf( map["cset_s1"] + " SPCA" ) >= 0), "Expect s1 to be SPCA.");

	//              6   7
	//               \ /
	//                8

	this.do_exec_merge(map["cset_6"], 0, "");

	vscript_test_wc__print_section_divider("After 6-into-7 merge - expecting edit conflict with per-file ancestor:");

	var expect_test = new Array;
	expect_test["Added (B)"] = [ "@/file_x1" ];
	expect_test["Added (C)"] = [ "@/file_x0" ];
	expect_test["Added (B,C)"] = [ "@/file_0", "@/file_3", "@/file_5" ];
	expect_test["Modified (WC!=B,WC!=C,B!=C)"] = [ "@/file_0" ];
	expect_test["Unresolved"] = [ "@/file_0" ];
	expect_test["Choice Unresolved (Contents)"] = [ "@/file_0" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	var issue = sg.wc.get_item_issues({"src": "@/file_0"});
	// since file_0 is not present in 4, we let the aux-ancestor code try to
	// find the "best" ancestor for the file.  it is present in SPCAs s0 and
	// s1 but these are peers and disqualified.  the aux-ancestor code will
	// also identify {3, 4, 5} as a set of peers. 4 is disqualified because
	// the item isn't present in it.  3 and 5 are disqualified for the same
	// reason as s0 and s1.  So we dig deeper and find 0f as the unique "best"
	// ancestor.
	testlib.ok( (issue.input.A.cset_hid == map["cset_0f"]), "Expect AUX-ANCESTOR for file_0.");

    }

}
