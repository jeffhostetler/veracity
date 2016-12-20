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
// Do some combinations of "vv diff" and "vv move" or "vv rename"
// for sprawl-642 "diff fails on a file that's been modified and moved".
//
//////////////////////////////////////////////////////////////////

function st_move_diff_1()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_main = function()
	{
		// create some basic files and directories.
		// and first real commit.

        this.do_fsobj_create_file("file_0");
        this.do_fsobj_mkdir("dir_A");
        this.do_fsobj_mkdir("dir_A/dir_1");
        this.do_fsobj_create_file("dir_A/dir_1/file_1");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs");

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

		//////////////////////////////////////////////////////////////////
		print("");
		print("//////////////////////////////////////////////////////////////////");
		print("");
		//////////////////////////////////////////////////////////////////
		// modify the files and verify status.

		this.do_fsobj_append_file("file_0", "Hello World!");
		this.do_fsobj_append_file("dir_A/dir_1/file_1", "Good Bye!");

        var expect_test = new Array;
		expect_test["Modified"] = [ "@/file_0",
									"@/dir_A/dir_1/file_1" ];
        vscript_test_wc__confirm_wii(expect_test);

		// confirm that 'vv diff' doesn't have any problems.

		var r1 = sg.exec(vv, "diff", "file_0");
		print(dumpObj(r1, "Simple Diff Result", "", 0));
		// TODO 2011/02/02 should "vv diff" exit with 1 when there are
		// TODO            differences (like gnu diff) (and exit with 0
		// TODO            when there are no differences) ?
		testlib.ok( (r1.exit_status == 0),
					"Expect differences in file_0" );
		// we expect output like this:
		// === ================
		// ===   Modified: File @/file_0
		// --- @/file_0 ...
		// +++ @/file_0 ...
		testlib.ok( (r1.stdout.search("===   Modified: File @/file_0") > 0),
					"Expect modified diff header for file_0" );

		//////////////////////////////////////////////////////////////////
		print("");
		print("//////////////////////////////////////////////////////////////////");
		print("");
		//////////////////////////////////////////////////////////////////
		// move one of the files and confirm that status is correct.

		vscript_test_wc__move("file_0", "dir_A");

        var expect_test = new Array;
		expect_test["Modified"] = [ "@/dir_A/file_0",
									"@/dir_A/dir_1/file_1" ];
		expect_test["Moved"]    = [ "@/dir_A/file_0" ];
        vscript_test_wc__confirm_wii(expect_test);

		// confirm that 'vv diff' doesn't have any problems.

		var r2 = sg.exec(vv, "diff", "dir_A/file_0");
		print(dumpObj(r2, "Move+Diff Result", "", 0));
		// TODO 2011/02/02 should "vv diff" exit with 1 when there are
		// TODO            differences (like gnu diff) (and exit with 0
		// TODO            when there are no differences) ?
		testlib.ok( (r2.exit_status == 0),
					"Expect differences in dir_A/file_0" );

		// we expect output like this:
		// === ================
		// ===      Moved: File @/dir_A/file_0 (from @/)
		// ===   Modified: File @/dir_A/file_0
		// --- @/dir_A/file_0 ...
		// +++ @/dir_A/file_0 ...
		//
		// we omit " (from @/)" from the moved line search pattern
		// because it messes with the search.
		testlib.ok( (r2.stdout.search("===      Moved: File @/dir_A/file_0") > 0),
					"Expect moved diff header for dir_A/file_0" );
		testlib.ok( (r2.stdout.search("===   Modified: File @/dir_A/file_0") > 0),
					"Expect modified diff header for dir_A/file_0" );

	}
}
