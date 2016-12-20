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

function st_merge_unresolved_1()
{
	load("update_helpers.js");			// load the helper functions
	initialize_update_helpers(this);	// initialize helper functions

	//////////////////////////////////////////////////////////////////

	this.setUp = function()
	{
		this.workdir_root = sg.fs.getcwd();		// save this for later

		//////////////////////////////////////////////////////////////////
		// fetch the HID of the initial commit.

		this.csets.push( sg.wc.parents()[0] );
		this.names.push( "*Initial Commit*" );

		//////////////////////////////////////////////////////////////////
		// create 3 csets:
		//
		//		0
		//	   /
		//	  1
		//	 /
		//	2
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
		vscript_test_wc__addremove();
		this.do_fsobj_append_file("dir_0/dir_00/file_000", "TRUNK CONFLICT");	// edit file_000
		this.do_commit("Simple ADDs 2");

		this.rev_trunk_head = this.csets[ this.csets.length - 1 ];

		//////////////////////////////////////////////////////////////////
		// update back to '1' and start a new branch '3':
		//		0
		//	   /
		//	  1
		//	 / \
		//	2	3

		this.do_update_when_clean_by_name("Simple ADDs 1");
		this.do_fsobj_mkdir("dir_3");
		vscript_test_wc__addremove();
		this.do_fsobj_append_file("dir_0/dir_00/file_000", "BRANCH CONFLICT");	// edit file_000
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
	//		 compute a STATUS and compare that with the expected results.
	//		 A STATUS reflects the changes made to the WD from the
	//		 BASELINE.	So the STATUS should show how the FINAL RESULT
	//		 changed from the BASELINE.
	//
	//		 This feels a little odd because we are accustomed to
	//		 think about the total net changes from the ANCESTOR to
	//		 the FINAL RESULT.
	//
	//		 So, for example, if a file was renamed between the
	//		 ancestor and the baseline branch (but not in the other
	//		 changeset), it will not show up in the status.	 It will
	//		 have the new name in the final result, but that is not
	//		 a change relative to the baseline.
	//////////////////////////////////////////////////////////////////

	this.test_simple_tb__merge_tool = function()
	{
		this.force_clean_WD();

		// set WD to the trunk.
		// merge (fold) in the branch.
		// make sure basic clean merge works as expected.

		this.do_update_by_rev(this.rev_trunk_head);
		this.do_exec_merge(this.rev_branch_head, 0, "");

		var expect_test = new Array;
		expect_test["Added (Merge)"]	 = [ "@/dir_3" ];
		expect_test["Modified"]	         = [ "@/dir_0/dir_00/file_000" ];
		expect_test["Auto-Merged"]	 = [ "@/dir_0/dir_00/file_000" ];
		expect_test["Unresolved"]	 = [ "@/dir_0/dir_00/file_000" ];
		expect_test["Choice Unresolved (Contents)"]	 = [ "@/dir_0/dir_00/file_000" ];
		vscript_test_wc__confirm_wii(expect_test);
	}

	this.test_simple_bt__merge_tool = function()
	{
		this.force_clean_WD();

		// set WD to the branch.
		// merge (fold) in the trunk.
		// make sure basic clean merge works as expected.

		this.do_update_by_rev(this.rev_branch_head);
		this.do_exec_merge(this.rev_trunk_head, 0, "");

		var expect_test = new Array;
		expect_test["Added (Merge)"]	 = [ "@/dir_2" ];
		expect_test["Modified"]	         = [ "@/dir_0/dir_00/file_000" ];
		expect_test["Auto-Merged"]	 = [ "@/dir_0/dir_00/file_000" ];
		expect_test["Unresolved"]	 = [ "@/dir_0/dir_00/file_000" ];
		expect_test["Choice Unresolved (Contents)"]	 = [ "@/dir_0/dir_00/file_000" ];
		vscript_test_wc__confirm_wii(expect_test);
	}
/*
 * Removed these tests after removing the --tool flag from merge,
 * since they rely on the behavior of the :skip tool.  The do_exec_merge
 * function no longer accepts a tool argument.
 *
	this.test_simple_tb__no_merge = function()
	{
		this.force_clean_WD();

		// set WD to the trunk.
		// merge (fold) in the branch.
		// make sure basic clean merge works as expected.

		this.do_update_by_rev(this.rev_trunk_head);
		this.do_exec_merge(this.rev_branch_head, 0, ":skip", "");

		var expect_test = new Array;
		expect_test["Added"]	 = [ "@/dir_3" ];
		// both branches modified the file and created a conflict, but :no_merge
		// does not attempt an auto-merge of the content.
		// 
		// TODO 2011/06/27 See S00085.  JS status does not contain a ["Conflict"] section.
		// TODO expect_test["Conflict"]	     = [ "@/dir_0/dir_00/file_000" ];
		vscript_test_wc__confirm_wii(expect_test);

		// Independently confirm the conflict until S00085 is fixed.
		var s = sg.exec(vv, "status");
		testlib.ok( (s.stdout.search("Conflict:  @/dir_0/dir_00/file_000") >= 0),
			    "Expecting conflict in output.  Found:\n" + s.stdout);


	}

	this.test_simple_bt__no_merge = function()
	{
		this.force_clean_WD();

		// set WD to the branch.
		// merge (fold) in the trunk.
		// make sure basic clean merge works as expected.

		this.do_update_by_rev(this.rev_branch_head);
		this.do_exec_merge(this.rev_trunk_head, 0, ":skip", "");

		var expect_test = new Array;
		expect_test["Added"]	 = [ "@/dir_2" ];
		// TODO 2011/06/27 See S00085.  JS status does not contain a ["Conflict"] section.
		// TODO expect_test["Conflict"]	     = [ "@/dir_0/dir_00/file_000" ];
		vscript_test_wc__confirm_wii(expect_test);

		// Independently confirm the conflict until S00085 is fixed.
		var s = sg.exec(vv, "status");
		testlib.ok( (s.stdout.search("Conflict:  @/dir_0/dir_00/file_000") >= 0),
			    "Expecting conflict in output.  Found:\n" + s.stdout);
	}
*/
}
