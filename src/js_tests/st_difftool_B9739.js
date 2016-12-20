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

function st_difftool_B9739()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_B9739 = function()
    {
	var cset_0 = sg.wc.parents()[0];	// the initial commit

	this.do_fsobj_create_file("file_1.txt", "Hello!\n");
	this.do_fsobj_create_file("file_2.txt", "Hello Again!\n");
	vscript_test_wc__addremove();
	var cset_1 = vscript_test_wc__commit("Add file.");

	// make some dirt in the WD.
	this.do_fsobj_create_file("file_1.txt", "Good Bye!\n");
	this.do_fsobj_create_file("file_2.txt", "Good Bye!\n");

	//////////////////////////////////////////////////////////////////
	// I'm only doing the gnu-diff test on Mac/Linux because and not
	// on Windows because I don't want to have the Windows version
	// depend on cygwin or something.

	if ((sg.platform() == "LINUX") || (sg.platform() == "MAC"))
	{
	    vscript_test_wc__print_section_divider("WC: Use custom gnu diff tool");

	    var re1 = new RegExp("Modified:[ \t]*@/file_1.txt");
	    var re2 = new RegExp("Modified:[ \t]*@/file_2.txt");

	    var o = sg.exec(vv, "config", "reset",  "filetools/diff/gnu");
	    var o = sg.exec(vv, "config", "set",    "filetools/diff/gnu/path", "/usr/bin/diff");
	    var o = sg.exec(vv, "config", "reset",  "filetools/diff/gnu/args");
	    var o = sg.exec(vv, "config", "add-to", "filetools/diff/gnu/args", "\\-u", "@FROM@", "@TO@");
	    // gnu-diff exits with 1 with files are different.
	    // treat that like a "failure" (like when given a
	    // missing or binary file) rather than a "different".
	    var o = sg.exec(vv, "config", "set",    "filetools/diff/gnu/exit/1", "failure");

	    var r = sg.exec(vv, "diff", "--tool=gnu", "file_1.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( (re1.test(r.stdout)),
			"Expected modified line" );
	    testlib.ok( r.stderr.indexOf("The tool 'gnu' reported an error") >= 0,
			"Expect vv error message" );
	    testlib.ok( (r.exit_status != 0),
			"Expect error code from tool" );

	    var r = sg.exec(vv, "diff", "--tool=gnu", "file_2.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( (re2.test(r.stdout)),
			"Expected modified line" );
	    testlib.ok( r.stderr.indexOf("The tool 'gnu' reported an error") >= 0,
			"Expect vv error message" );
	    testlib.ok( (r.exit_status != 0),
			"Expect error code from tool" );

	    // With the changes for W3654, we try to let the diff run
	    // as long as possible and write per-file/tool errors to stderr
	    // and exit with 2 if there were any tool errors.  We still try
	    // to exit with 1 if there were differences.

	    var r = sg.exec(vv, "diff", "--tool=gnu");
	    print(sg.to_json__pretty_print(r));
	    var m1 = (re1.test(r.stdout));
	    var m2 = (re2.test(r.stdout));
	    testlib.ok( (m1 && m2),
			"Expected both modified files" );
	    testlib.ok( r.stderr.indexOf("The tool 'gnu' reported an error") >= 0,
			"Expect vv error message" );
	    testlib.ok( (r.exit_status == 2),
			"Expect error code from tool" );
	}

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Commit the changes and repeat tests using VV2 code.");
	var cset_2 = vscript_test_wc__commit("Edit files.");

	//////////////////////////////////////////////////////////////////
	if ((sg.platform() == "LINUX") || (sg.platform() == "MAC"))
	{
	    vscript_test_wc__print_section_divider("VV2: Use custom gnu diff tool");

	    var re1 = new RegExp("Modified:[ \t]*@1/file_1.txt");
	    var re2 = new RegExp("Modified:[ \t]*@1/file_2.txt");

	    print("");
	    var r = sg.exec(vv, "diff", "-r", cset_1, "-r", cset_2, "--tool=gnu", "file_1.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( (re1.test(r.stdout)),
			"Expected modified line" );
	    testlib.ok( r.stderr.indexOf("The tool 'gnu' reported an error") >= 0,
			"Expect vv error message" );
	    testlib.ok( (r.exit_status != 0),
			"Expect error code from tool" );

	    print("");
	    var r = sg.exec(vv, "diff", "-r", cset_1, "-r", cset_2, "--tool=gnu", "file_2.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( (re2.test(r.stdout)),
			"Expected modified line" );
	    testlib.ok( r.stderr.indexOf("The tool 'gnu' reported an error") >= 0,
			"Expect vv error message" );
	    testlib.ok( (r.exit_status != 0),
			"Expect error code from tool" );

	    print("");
	    var r = sg.exec(vv, "diff", "-r", cset_1, "-r", cset_2, "--tool=gnu");
	    print(sg.to_json__pretty_print(r));
	    var m1 = (re1.test(r.stdout));
	    var m2 = (re2.test(r.stdout));
	    testlib.ok( (m1 && m2),
			"Expected both modified files" );
	    testlib.ok( r.stderr.indexOf("The tool 'gnu' reported an error") >= 0,
			"Expect vv error message" );
	    testlib.ok( (r.exit_status == 2),
			"Expect error code from tool" );

	}

    }
}
