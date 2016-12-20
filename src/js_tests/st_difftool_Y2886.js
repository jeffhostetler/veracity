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

function st_difftool_Y2886()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_Y2886 = function()
    {
	var cset_0 = sg.wc.parents()[0];	// the initial commit

	this.do_fsobj_create_file("file_1.txt", "Hello!\n");
	this.do_fsobj_create_file("file_2.gif", "Hello!\n");	// just want the suffix to get :skip to run
	vscript_test_wc__addremove();
	var cset_1 = vscript_test_wc__commit("Add file.");

	// make some dirt in the WD.
	this.do_fsobj_create_file("file_1.txt", "Good Bye!\n");
	this.do_fsobj_create_file("file_2.gif", "Good Bye!\n");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Use internal built-in diff tool");

	var r = sg.exec(vv, "diff", "file_1.txt");
	print(sg.to_json__pretty_print(r));
	testlib.ok( r.exit_status == 0 );

	//////////////////////////////////////////////////////////////////
	// I'm only doing the gnu-diff test on Mac/Linux because and not
	// on Windows because I don't want to have the Windows version
	// depend on cygwin or something.

	if ((sg.platform() == "LINUX") || (sg.platform() == "MAC"))
	{
	    vscript_test_wc__print_section_divider("Use custom gnu diff tool");

	    var o = sg.exec(vv, "config", "reset",  "filetools/diff/gnu");
	    var o = sg.exec(vv, "config", "set",    "filetools/diff/gnu/path", "/usr/bin/diff");

	    print("");
	    print("try using gnu tool without setting 'filetools/diff/gnu/args'...");
	    var r = sg.exec(vv, "diff", "--tool=gnu", "file_1.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( r.stderr.indexOf("The 'args' setting for the tool 'gnu' has not been set") >= 0,
			"Expect error" );

	    print("");
	    print("try using gnu tool with 'filetools/diff/gnu/args' incorrectly set as a string field...");
	    var o = sg.exec(vv, "config", "set",    "filetools/diff/gnu/args", "\\-u @FROM@ @TO@");
	    var r = sg.exec(vv, "diff", "--tool=gnu", "file_1.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( r.stderr.indexOf("The 'args' setting for the tool 'gnu' must be an array") >= 0,
			"Expect error" );

	    print("");
	    print("try using gnu tool with args set correctly, but no exit mapping (implicit 1 ==> failure)");
	    var o = sg.exec(vv, "config", "reset",  "filetools/diff/gnu/args");
	    var o = sg.exec(vv, "config", "add-to", "filetools/diff/gnu/args", "\\-u", "@FROM@", "@TO@");
	    var r = sg.exec(vv, "diff", "--tool=gnu", "file_1.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( r.stderr.indexOf("The tool 'gnu' reported an error") >= 0,
			"Expect error" );

	    print("");
	    print("try using gnu tool with exit mapping (1 ==> different)");
	    var o = sg.exec(vv, "config", "set",    "filetools/diff/gnu/exit/1", "different");
	    var r = sg.exec(vv, "diff", "--tool=gnu", "file_1.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( (r.exit_status == 0),
			"Expect error" );
	}

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("try diffing a 'binary' file...");

	var r = sg.exec(vv, "diff", "--tool=:skip", "file_2.gif");
	print(sg.to_json__pretty_print(r));
	testlib.ok( r.stderr.indexOf("The difftool was canceled") >= 0,
		    "Expect error" );
	testlib.ok( (r.exit_status == 2),
		    "Expect error" );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Commit the changes and repeat tests using VV2 code.");
	var cset_2 = vscript_test_wc__commit("Edit files.");

	//////////////////////////////////////////////////////////////////
	if ((sg.platform() == "LINUX") || (sg.platform() == "MAC"))
	{
	    vscript_test_wc__print_section_divider("VV2: Use custom gnu diff tool");

	    print("");
	    var r = sg.exec(vv, "diff", "-r", cset_1, "-r", cset_2, "--tool=gnu", "file_1.txt");
	    print(sg.to_json__pretty_print(r));
	    testlib.ok( (r.exit_status == 0),
			"Expect error" );
	}

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("VV2: try binary file...");

	print("");
	var r = sg.exec(vv, "diff", "-r", cset_1, "-r", cset_2, "--tool=:skip", "file_2.gif");
	print(sg.to_json__pretty_print(r));
	testlib.ok( r.stderr.indexOf("The difftool was canceled") >= 0,
		    "Expect error" );
	testlib.ok( (r.exit_status == 2),
		    "Expect error" );

    }
}
