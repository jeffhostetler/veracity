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

function st_revert__W8201()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        //////////////////////////////////////////////////////////////////
	// 3 versions of the contents of a file that can can 

	this.content_ancestor = ("This is line 1\n" +
				 "This is line 2\n" +
				 "This is line 3\n" +
				 "This is line 4\n" +
				 "This is line 5\n" +
				 "This is line 6\n" +
				 "This is line 7\n" +
				 "This is line 8\n" +
				 "This is line 9\n" );

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
	this.do_fsobj_create_file_exactly("file_0.txt", this.content_ancestor);
	this.do_fsobj_create_file_exactly("file_1.txt", this.content_ancestor);
	this.do_fsobj_create_file_exactly("file_2.txt", this.content_ancestor);
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 0");

        this.do_fsobj_mkdir("dir_1");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 1");

        this.do_fsobj_mkdir("dir_2");
        vscript_test_wc__addremove();

	this.do_fsobj_append_file("file_0.txt",                  "In cset 2.\n");
	vscript_test_wc__rename(  "file_0.txt",                  "file_0_renamed_in_cset2.txt" );
	vscript_test_wc__move(    "file_0_renamed_in_cset2.txt", "dir_2"  );

	this.do_fsobj_append_file("file_1.txt",                  "In cset 2.\n");
	vscript_test_wc__rename(  "file_1.txt",                  "file_1_renamed_in_cset2.txt" );
	vscript_test_wc__move(    "file_1_renamed_in_cset2.txt", "dir_2"  );

	this.do_fsobj_append_file("file_2.txt",                  "In cset 2.\n");
	vscript_test_wc__rename(  "file_2.txt",                  "file_2_renamed_in_cset2.txt" );
	vscript_test_wc__move(    "file_2_renamed_in_cset2.txt", "dir_2"  );
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
	sg.exec(vv, "branch", "attach", "master");
        this.do_fsobj_mkdir("dir_3");
        vscript_test_wc__addremove();

	this.do_fsobj_append_file("file_0.txt",                  "In cset 3.\n");
	vscript_test_wc__rename(  "file_0.txt",                  "file_0_renamed_in_cset3.txt" );
	vscript_test_wc__move(    "file_0_renamed_in_cset3.txt", "dir_3"  );

	this.do_fsobj_append_file("file_1.txt",                  "In cset 3.\n");
	vscript_test_wc__rename(  "file_1.txt",                  "file_1_renamed_in_cset3.txt" );
	vscript_test_wc__move(    "file_1_renamed_in_cset3.txt", "dir_3"  );

	this.do_fsobj_append_file("file_2.txt",                  "In cset 3.\n");
	vscript_test_wc__rename(  "file_2.txt",                  "file_2_renamed_in_cset3.txt" );
	vscript_test_wc__move(    "file_2_renamed_in_cset3.txt", "dir_3"  );
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
    }

    this.test_simple_tb_1 = function()
    {
	this.force_clean_WD();

	// set WD to the trunk.
	// merge (fold) in the branch.
	// make sure basic clean merge works as expected.

	this.do_update_by_rev(this.rev_trunk_head);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Starting MERGE...");
	print("");

	this.do_exec_merge(this.rev_branch_head, 0, "");

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3" ];
	expect_test["Modified"]      = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
					 "@b/dir_2/file_1_renamed_in_cset2.txt",
					 "@b/dir_2/file_2_renamed_in_cset2.txt" ];
	// there is a divergent move, but MERGE leaves it in the baseline directory
	// so it won't appear here.
	//expect_test["Moved"]       = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
	//                               "@b/dir_2/file_1_renamed_in_cset2.txt",
	//                               "@b/dir_2/file_2_renamed_in_cset2.txt" ];
	// the rename is here because MERGE give conflicts a temp name.
	expect_test["Renamed"]       = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
					 "@b/dir_2/file_1_renamed_in_cset2.txt",
					 "@b/dir_2/file_2_renamed_in_cset2.txt" ];
	expect_test["Auto-Merged"]   = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
					 "@b/dir_2/file_1_renamed_in_cset2.txt",
					 "@b/dir_2/file_2_renamed_in_cset2.txt" ];
	expect_test["Unresolved"]    = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
					 "@b/dir_2/file_1_renamed_in_cset2.txt",
					 "@b/dir_2/file_2_renamed_in_cset2.txt" ];
	expect_test["Choice Unresolved (Name)"]     = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
							"@b/dir_2/file_1_renamed_in_cset2.txt",
							"@b/dir_2/file_2_renamed_in_cset2.txt" ];
	expect_test["Choice Unresolved (Location)"] = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
							"@b/dir_2/file_1_renamed_in_cset2.txt",
							"@b/dir_2/file_2_renamed_in_cset2.txt" ];
	expect_test["Choice Unresolved (Contents)"] = [ "@b/dir_2/file_0_renamed_in_cset2.txt",
							"@b/dir_2/file_1_renamed_in_cset2.txt",
							"@b/dir_2/file_2_renamed_in_cset2.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// RESOLVE some of the files (because we want to eventually test
	// whether or not REVERT-ALL will cleanup the ISSUES.

	vscript_test_wc__revert_all__np( { "no_backups" : true } );

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

    }

}
