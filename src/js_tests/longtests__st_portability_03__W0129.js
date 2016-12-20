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

function st_portability_03()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    // this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	this.do_commit("Added AAA");

	// branch 1 -- do rename

	vscript_test_wc__rename("AAA", "xxx");	// 2-step rename (see W6834)
	vscript_test_wc__rename("xxx", "aaa");
	this.do_commit("Renamed AAA to aaa");

// I fixed H6890 and SPRAWL-915 yesterday.
//	// Because of H6890 and SPRAWL-915, we can't just
//	// update back to "Added AAA", so we do another
//	// 2-step rename back to the original, commit that
//	// and then update back.
//
//	vscript_test_wc__rename("aaa", "xxx");	// 2-step rename (see W6834)
//	vscript_test_wc__rename("xxx", "AAA");
//	this.do_commit("HACK Renamed aaa to AAA");

	this.do_update_when_clean_by_name("Added AAA");
	sg.exec(vv, "branch", "attach", "master");

	// start branch 2 with an unrelated change.

	this.do_fsobj_mkdir("BBB");
	vscript_test_wc__addremove();
	this.do_commit("Added BBB");

	// try to merge branch 1 as of the first rename cset
	// into branch 2 (causing the MERGE code to detect
	// and implictly do a 2-step rename to get "AAA" to "aaa").

	var ndx = this.name2ndx["Renamed AAA to aaa"];
	var rev = this.csets[ndx];
	print("vv merge --rev=" + rev);
	var r = sg.exec(vv, "merge", "--verbose", "--rev="+rev);
	print("STDOUT:\n" + r.stdout);
	print("STDERR:\n" + r.stderr);
	if (r.exit_status != 0)
	{
	    testlib.ok( r.exit_status == 0, "Merge exit status should be 0" );
	}
	else
	{
	    var expect_test = new Array;
	    expect_test["Renamed"] = [ "@/aaa" ];
            vscript_test_wc__confirm_wii(expect_test);
	}

    }

}
