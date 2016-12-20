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
// Test COMMIT of a DELETED item by name.
//////////////////////////////////////////////////////////////////

function st_remove_deleted__R5906()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

        this.do_fsobj_create_file("file_1");
        this.do_fsobj_create_file("file_2");
        this.do_fsobj_create_file("file_3");
        this.do_fsobj_create_file("file_4");
        this.do_fsobj_create_file("file_5");
	vscript_test_wc__addremove();
	this.do_commit("add initial files");
	var cset_base = sg.wc.parents()[0];

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Deleting something....");
	print("");

	vscript_test_wc__remove_file("file_1");
	vscript_test_wc__remove_file("file_2");
	vscript_test_wc__remove_file("file_3");

        var expect_test = new Array;
	expect_test["Removed"] = [ "@b/file_1",
				   "@b/file_2",
				   "@b/file_3"
				 ];
        vscript_test_wc__confirm_wii(expect_test);

	testlib.ok( (sg.fs.exists("file_1")==false), "file_1 should not exist.");

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Commit one deleted file by name....");
	print("");

	// 2012/01/06 We *MUST* use the new expanded repo-path prefix
	//            for the baseline to name the deleted item because
	//            the deleted item no longer "owns" the name in the
	//            current/live directory.
	//            See W4908.

	var result = sg.exec(vv, "commit", "-m", "commit_the_delete", "@b/file_1");
	testlib.ok( (result.exit_status == 0), "'vv commit file_1'" );
	if (result.exit_status != 0)
	{
	    print("Received:");
	    print(result.stderr);
	}

	// remaining changes should still be present.

        var expect_test = new Array;
	expect_test["Removed"] = [ "@b/file_2",
				   "@b/file_3"
				 ];
        vscript_test_wc__confirm_wii(expect_test);


    }

}
