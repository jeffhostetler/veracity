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
// Tests REMVOE using --exclude
//////////////////////////////////////////////////////////////////
/*
function st_remove__X7399_01()
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
	print("Try to REMOVE using --exclude...");
	print("");

	var result = sg.exec(vv, "remove", "--exclude=file_2", "file_1");
	if (result.exit_status != 0)
	{
	    testlib.ok( (result.exit_status == 0), "vv remove should return zero status.  Received: " + result.exit_status );
	    print(dumpObj(result, "sg.exec()", "", 0));
	}

        var expect_test = new Array;
	expect_test["Removed"] = [ "@/file_1" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Fake a 'vv remove * --exclude file_2 .... (because we don't have shell to do globbing for us)");
	print("");

	var result = sg.exec(vv, "remove", "--exclude=file_2", "file_2", "file_3", "file_4");
	if (result.exit_status != 0)
	{
	    testlib.ok( (result.exit_status == 0), "vv remove should return zero status.  Received: " + result.exit_status );
	    print(dumpObj(result, "sg.exec()", "", 0));
	}

        var expect_test = new Array;
	expect_test["Removed"] = [ "@/file_1",
				   "@/file_3",
				   "@/file_4" ];
        vscript_test_wc__confirm_wii(expect_test);

    }

}
*/
