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

function st_W4060_01()
{
    var map = new Array();

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.my_do_commit = function(label)
    {
	this.do_commit(label);
	map[label] = sg.wc.parents()[0];
	sg.exec(vv, "tag", "add", label);
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
	//
	//     	       	A
	//	       / \
	//            /   0
	//           B   / \
	//          /   1   2
	//         /   / \ / \
	//        C   3   4   5
	//         \   \ / \ /
	//          \  6a   7
	//           \ /
	//            6
	//
	// [] start with a file that exists in 0, 1, and 2.
	// [] modify the file in 3 and 5.
	// [] delete the file during the merge creating 4.
	// [] accept the modified (rather than deleted) version
	//    of the file when creating merges 6 and 7.
	//
	// When we try to merge 6 and 7, we consider this
	// portion of the DAG.
	//
	//                4
	//               / \
	//              6   7
	//               \ /
	//                8
	//
	// The file exists in both 6 and 7 (with the same GID),
	// but it does not exist in 4.  So there is no ancestor
	// version available.  This causes the MERGE code to
	// throw with a duplicate (GID) key because it looks
	// like BOTH sides ADDED it.
	//
	// The test in st_W4060.js should have found the path
	// from 6 --> 3 --> ... --> 0.
	//
	// The purpose of st_W4060_01.js is to show that we
	// can't blindly choose the first alternate path we
	// find.  That is, if we go 6 --> C --> ... --> A
	// we get an aux-lca where the file doesn't yet exist.
	// So the MERGE is no better (FOR THIS INDIVIDUAL FILE)
	// than when we just considered 4.

        this.do_fsobj_create_file("file_A");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_A");

        this.do_fsobj_create_file("file_B");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_B");

        this.do_fsobj_create_file("file_C");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_C");

	this.do_update_by_rev_attached(map["cset_A"], "master");

        this.do_fsobj_create_file("file_0");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_0");

        this.do_fsobj_create_file("file_1");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_1");

        this.do_fsobj_create_file("file_3");
        vscript_test_wc__addremove();
	this.do_fsobj_append_file("file_0", "SAME MESSAGE IN 3 and 5.\n");
        this.my_do_commit("cset_3");

	this.do_update_by_rev_attached(map["cset_0"], "master");

        this.do_fsobj_create_file("file_2");
        vscript_test_wc__addremove();
        this.my_do_commit("cset_2");

        this.do_fsobj_create_file("file_5");
        vscript_test_wc__addremove();
	this.do_fsobj_append_file("file_0", "SAME MESSAGE IN 3 and 5.\n");
        this.my_do_commit("cset_5");

	this.do_update_by_rev_attached(map["cset_1"], "master");

	this.do_exec_merge(map["cset_2"], 0, "");
	vscript_test_wc__remove_file("file_0");
	this.my_do_commit("cset_4");

	this.do_exec_merge(map["cset_3"], 0, "");
	sg.exec(vv, "resolve", "accept", "other", "--all");
	this.my_do_commit("cset_6a");

	this.do_exec_merge(map["cset_C"], 0, "");
	sg.exec(vv, "resolve", "accept", "other", "--all");
	this.my_do_commit("cset_6");

	this.do_update_by_rev_attached(map["cset_4"], "master");

	this.do_exec_merge(map["cset_5"], 0, "");
	sg.exec(vv, "resolve", "accept", "other", "--all");
	this.my_do_commit("cset_7");

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);
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

    this.test_simple_1 = function()
    {
	var o = sg.exec(vv, "dump_lca", "-r", map["cset_6"], "-r", map["cset_7"], "--list");
	print("daglca(6,7):");
	print(o.stdout);
	var regexLCA = new RegExp("(.*) LCA");
	var matches = o.stdout.match(regexLCA);
	testlib.ok( (matches), "Expect LCA" );
	var lca = matches[1];
	testlib.ok( (lca == map["cset_4"]), "Expect LCA to be cset_4.");

	//              6   7
	//               \ /
	//                8

	this.do_exec_merge(map["cset_6"], 0, "");
	this.my_do_commit("cset_8");

    }

}
