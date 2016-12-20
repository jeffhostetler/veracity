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
// Some basic tests for doing arbitrary (unrelated) updates when
// there is dirt.  This test is intended to be exactly the same
// as __V00033_0() but with the item found rather than ignored.
//////////////////////////////////////////////////////////////////

function st_unrelated_dirty_update_V00033_0a()
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
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 0");

        this.do_fsobj_mkdir("dir_1");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 1");

        this.do_fsobj_mkdir("dir_2");
        vscript_test_wc__addremove();
        this.do_commit("Simple ADDs 2");

	// these should be equivalent
	// this.rev_trunk_head = this.csets[ this.csets.length - 1 ];
	this.rev_trunk_head = sg.wc.parents()[0];

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
        this.do_commit_in_branch("Simple ADDs 3");

	// these should be equivalent
	// this.rev_branch_head = this.csets_in_branch[ this.csets_in_branch.length - 1 ];
	this.rev_branch_head = sg.wc.parents()[0];

	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);
    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	// we are looking at [3] the value in this.rev_branch_head.

	// create some dirt (no one said it had to be created
	// by a previous command like revert)

	this.do_fsobj_create_file("foo.c__not_ignored");

        var expect_test = new Array;
        expect_test["Found"] = [ "@/foo.c__not_ignored" ];
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("confirm that the file is NOT ignorable");

	// confirm that the file is NOT ignorable.

        var expect_test = new Array;
        expect_test["Found"] = [ "@/foo.c__not_ignored" ];
        vscript_test_wc__confirm(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("try to do an unrelated (non ancestor/descendant) update to [1].");

	vscript_test_wc__update( this.rev_trunk_head );

        var expect_test = new Array;
        expect_test["Found"] = [ "@/foo.c__not_ignored" ];
        vscript_test_wc__confirm_wii(expect_test);
    }
}
