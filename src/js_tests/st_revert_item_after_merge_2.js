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

function st_revert_item_after_merge_2()
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

	this.content_trunk    = ("This is line 1\n" +
				 "This is line 2\n" +
				 "TRUNK\n" +
				 "This is line 3\n" +
				 "This is line 4\n" +
				 "This is line 5\n" +
				 "This is line 6\n" +
				 "This is line 7\n" +
				 "This is line 8\n" +
				 "This is line 9\n" );

	this.content_branch   = ("This is line 1\n" +
				 "This is line 2\n" +
				 "This is line 3\n" +
				 "This is line 4\n" +
				 "This is line 5\n" +
				 "This is line 6\n" +
				 "BRANCH\n" +
				 "This is line 7\n" +
				 "This is line 8\n" +
				 "This is line 9\n" );

        //////////////////////////////////////////////////////////////////
	// because of bug F00050, we cannot create a binary file and have
	// the builtin :binary tool to get selected during the merge.  so
	// for now, define our own tool for a fake binary file type.

	// define a new class with only one suffix
	sg.exec(vv, "config", "empty",  "fileclasses/fake_binary_class/patterns");
	sg.exec(vv, "config", "add-to", "fileclasses/fake_binary_class/patterns", "*.fake_bin");
	// don't allow auto-merge during MERGE
	sg.exec(vv, "config", "set", "filetoolbindings/merge/fake_binary_class/merge", ":skip");
	// allow merge during RESOLVE
	sg.exec(vv, "config", "set", "filetoolbindings/merge/fake_binary_class/resolve", ":merge");

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
	this.do_fsobj_create_file_exactly("merge_file_0.txt", this.content_ancestor);
	this.do_fsobj_create_file_exactly("merge_edited_file_0.txt", this.content_ancestor);
	this.do_fsobj_create_file_exactly("merge_file_0.fake_bin", this.content_ancestor);	// fake a binary file
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
	this.do_fsobj_create_file_exactly("merge_file_0.txt", this.content_trunk);	// overwrite to fake editing in this branch
	this.do_fsobj_create_file_exactly("merge_edited_file_0.txt", this.content_trunk);	// overwrite to fake editing in this branch
	this.do_fsobj_create_file_exactly("merge_file_0.fake_bin", this.content_trunk);
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
        this.do_fsobj_mkdir("dir_3/dir_33");
        this.do_fsobj_create_file("file_3");
        this.do_fsobj_create_file("dir_3/file_33");
        this.do_fsobj_create_file("dir_3/dir_33/file_333");
	vscript_test_wc__remove_file("dir_1/dir_11/file_111");
	vscript_test_wc__remove_dir("dir_1/dir_11");
        vscript_test_wc__addremove();
	this.do_fsobj_create_file_exactly("merge_file_0.txt", this.content_branch);	// overwrite to fake editing in this branch
	this.do_fsobj_create_file_exactly("merge_edited_file_0.txt", this.content_branch);	// overwrite to fake editing in this branch
	this.do_fsobj_create_file_exactly("merge_file_0.fake_bin", this.content_branch);
        this.do_commit_in_branch("Simple ADDs 3");

	this.rev_branch_head = this.csets_in_branch[ this.csets_in_branch.length - 1 ];

	//////////////////////////////////////////////////////////////////

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);
    }

    this.tearDown = function()
    {
	sg.exec(vv, "config", "reset", "fileclasses/fake_binary_class");
	sg.exec(vv, "config", "reset", "filetoolbindings/merge/fake_binary_class");
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

	// merge_file_0.txt should have been auto-merged by internal :merge
	// merge_file_0.fake_bin should be an unresolved issue because it got
	// treated as a binary file and handled by :skip.

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_3/dir_33",
					 "@/file_3",
					 "@/dir_3/file_33", 
					 "@/dir_3/dir_33/file_333" ];
	expect_test["Removed"]       = [ "@b/dir_1/dir_11",
					 "@b/dir_1/dir_11/file_111" ];
	expect_test["Modified"]      = [ "@/merge_file_0.txt",
					 "@/merge_edited_file_0.txt" ];
	expect_test["Auto-Merged"]   = [ "@/merge_file_0.txt",
					 "@/merge_edited_file_0.txt" ];
	expect_test["Resolved"]                     = [ "@/merge_file_0.txt",
							"@/merge_edited_file_0.txt" ];
	expect_test["Unresolved"]                   = [ "@/merge_file_0.fake_bin" ];
	expect_test["Choice Resolved (Contents)"]   = [ "@/merge_file_0.txt",
							"@/merge_edited_file_0.txt" ];
	expect_test["Choice Unresolved (Contents)"] = [ "@/merge_file_0.fake_bin" ];

	// With the fix for R00031, we now leave the baseline version of the 
	// binary file as is (rather than moving it to a backup file), so it
	// won't appear as modified/lost/found in the status.
        vscript_test_wc__confirm_wii(expect_test);

	// create some post-merge dirt.

	vscript_test_wc__move(   "dir_2/dir_22/file_222", "dir_3/dir_33" );
	vscript_test_wc__move(   "dir_2/file_22",         "dir_3/dir_33" );
	vscript_test_wc__remove( "dir_2/dir_22" );

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@/dir_3",
					 "@/dir_3/dir_33",
					 "@/file_3",
					 "@/dir_3/file_33", 
					 "@/dir_3/dir_33/file_333" ];
	expect_test["Removed"]       = [ "@b/dir_1/dir_11",
					 "@b/dir_1/dir_11/file_111",
					 "@b/dir_2/dir_22" ];
	expect_test["Moved"]         = [ "@/dir_3/dir_33/file_222",
					 "@/dir_3/dir_33/file_22" ];
	expect_test["Modified"]      = [ "@/merge_file_0.txt",
					 "@/merge_edited_file_0.txt" ];
	expect_test["Auto-Merged"]   = [ "@/merge_file_0.txt",
					 "@/merge_edited_file_0.txt" ];
	expect_test["Resolved"]                     = [ "@/merge_file_0.txt",
							"@/merge_edited_file_0.txt" ];
	expect_test["Unresolved"]                   = [ "@/merge_file_0.fake_bin" ];
	expect_test["Choice Resolved (Contents)"]   = [ "@/merge_file_0.txt",
							"@/merge_edited_file_0.txt" ];
	expect_test["Choice Unresolved (Contents)"] = [ "@/merge_file_0.fake_bin" ];
	// With the fix for R00031, we now leave the baseline version of the 
	// binary file as is (rather than moving it to a backup file), so it
	// won't appear as modified/lost/found in the status.
        vscript_test_wc__confirm_wii(expect_test);

	// this should fail because the item's baseline parent
	// has been deleted and isn't in the revert-set.
	vscript_test_wc__revert_items__expect_error( [ "dir_3/dir_33/file_222" ],
						     "Revert cannot move/restore item because of missing/deleted parent directory" );

	// another way of saying the same thing.
	vscript_test_wc__revert_items__expect_error( [ "dir_3" ],
						     "Revert cannot move/restore item because of missing/deleted parent directory" );

	// this should work (regardless of the ordering of the args)
	vscript_test_wc__revert_items( [ "dir_3", "dir_2" ] );

        var expect_test = new Array;
        expect_test["Added (Merge)"] = [ "@c/dir_3",
					 "@c/dir_3/dir_33",
					 "@/file_3",
					 "@c/dir_3/file_33", 
					 "@c/dir_3/dir_33/file_333" ];
	expect_test["Removed"]       = [ "@b/dir_1/dir_11",
					 "@b/dir_1/dir_11/file_111",
					 "@c/dir_3",
					 "@c/dir_3/dir_33",
					 "@c/dir_3/file_33", 
					 "@c/dir_3/dir_33/file_333" ];
	expect_test["Modified"]      = [ "@/merge_file_0.txt",
					 "@/merge_edited_file_0.txt" ];
	expect_test["Auto-Merged"]   = [ "@/merge_file_0.txt",
					 "@/merge_edited_file_0.txt" ];
	expect_test["Resolved"]                     = [ "@/merge_file_0.txt",
							"@/merge_edited_file_0.txt" ];
	expect_test["Unresolved"]                   = [ "@/merge_file_0.fake_bin" ];
	expect_test["Choice Resolved (Contents)"]   = [ "@/merge_file_0.txt",
							"@/merge_edited_file_0.txt" ];
	expect_test["Choice Unresolved (Contents)"] = [ "@/merge_file_0.fake_bin" ];
	// With the fix for R00031, we now leave the baseline version of the 
	// binary file as is (rather than moving it to a backup file), so it
	// won't appear as modified/lost/found in the status.
        vscript_test_wc__confirm_wii(expect_test);

    }

}
