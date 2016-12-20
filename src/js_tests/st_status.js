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
// Some basic unit tests for the STATUS command.
//////////////////////////////////////////////////////////////////

var globalThis;  // var to hold "this" pointers

function st_Status() 
{
	// Helper functions are in update_helpers.js
	// There is a reference list at the top of the file.
	// After loading, you must call initialize_update_helpers(this)
	// before you can use the helpers.
	load("update_helpers.js");			// load the helper functions
	initialize_update_helpers(this);	// initialize helper functions

	// this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

	//////////////////////////////////////////////////////////////////

	this.setUp = function()
	{
		this.workdir_root = sg.fs.getcwd();		// save this for later

		// Just start with an empty WD.	 Let each test create some
		// files/folders and do whatever testing and then clean-up
		// so that each individual test is independent and can be
		// run in isolation.
	}

	this.tearDown = function() 
	{
	}

    this.ignores = function(cmd, value) {
        var o;
        if (cmd == "reset") {
            o = sg.exec("vv", "localsetting", "reset", "ignores");
            if (o.exit_status != 0) 
            {
                print(sg.to_json(o));
                testlib.ok( (0), '## Unable to reset localsetting ignores.' );
                return false;
            }
        } else {
            o = sg.exec("vv", "localsetting", cmd, "ignores", value);
            if (o.exit_status != 0)
            {
                print(sg.to_json(o));
                testlib.ok( (0), '## Ignores: unable to ' + cmd + '"' + value + '"' );
                print(sg.exec("vv", "localsettings").stdout);
                return false;
            }
        }
        return true;
    }

	//////////////////////////////////////////////////////////////////

	this.test_status_1 = function test_status_1()
	{
		// test 1: baseline; make sure a list of directories are fully accounted for.

		this.ignores("reset");

		this.do_fsobj_mkdir("dir1a");
		this.do_fsobj_mkdir("dir1a/dir2a");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1a/dir2b");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b");
		this.do_fsobj_mkdir("dir1b/dir2a");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b/dir2b");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4/dir5");

		// verify that STATUS identifies ALL of our changes.

		var expect_all = new Array;
		expect_all["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1a/dir2b",
								"@/dir1a/dir2b/dir3",
								"@/dir1a/dir2b/dir3/dir4",
								"@/dir1a/dir2b/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5"
							  ];
		vscript_test_wc__confirm_wii(expect_all);

		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2b");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2a");
		this.do_fsobj_remove_dir("dir1b");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2b");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2a");
		this.do_fsobj_remove_dir("dir1a");

        var expect_nothing = new Array;
        vscript_test_wc__confirm_wii(expect_nothing);

		print("Reached bottom of " + 'test_status_1');
	}

	//////////////////////////////////////////////////////////////////

	this.test_status_2 = function test_status_2()
	{
		// test 2: add a series of "ignorable directories" to the tree and
		//         make sure that they are ignored when requested.

		this.ignores("reset");
		this.ignores("add-to", "*_ign");

		this.do_fsobj_mkdir("dir1a");
		this.do_fsobj_mkdir("dir1a/dir2a");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1a/dir2b");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b");
		this.do_fsobj_mkdir("dir1b/dir2a");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b/dir2b");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("foo1_ign");
		this.do_fsobj_mkdir("dir1a/foo2_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/foo3_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/foo4_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/foo5_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign");

		// verify that STATUS identifies ALL of our changes.

		var expect_all = new Array;
		expect_all["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1a/dir2b",
								"@/dir1a/dir2b/dir3",
								"@/dir1a/dir2b/dir3/dir4",
								"@/dir1a/dir2b/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5",
								"@/foo1_ign",
								"@/dir1a/foo2_ign",
								"@/dir1a/dir2a/foo3_ign",
								"@/dir1a/dir2a/dir3/foo4_ign",
								"@/dir1a/dir2a/dir3/dir4/foo5_ign",
								"@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign"
						  ];
		vscript_test_wc__confirm_wii(expect_all);

		// verify that a normal STATUS filters out the ignorables.

		var expect_ign = new Array;
		expect_ign["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1a/dir2b",
								"@/dir1a/dir2b/dir3",
								"@/dir1a/dir2b/dir3/dir4",
								"@/dir1a/dir2b/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5"
						  ];
		expect_ign["Ignored"] = [ "@/foo1_ign",
								  "@/dir1a/foo2_ign",
								  "@/dir1a/dir2a/foo3_ign",
								  "@/dir1a/dir2a/dir3/foo4_ign",
								  "@/dir1a/dir2a/dir3/dir4/foo5_ign",
								  "@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign"
						  ];

		vscript_test_wc__confirm(expect_ign);

		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/foo5_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/foo4_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/foo3_ign");
		this.do_fsobj_remove_dir("dir1a/foo2_ign");
		this.do_fsobj_remove_dir("foo1_ign");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2b");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2a");
		this.do_fsobj_remove_dir("dir1b");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2b");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2a");
		this.do_fsobj_remove_dir("dir1a");

        var expect_nothing = new Array;
        vscript_test_wc__confirm_wii(expect_nothing);

		print("Reached bottom of " + 'test_status_2');
	}

	//////////////////////////////////////////////////////////////////

	this.test_status_3 = function test_status_3()
	{
		// test 3: add a series of "ignorable directories" and add content
		//         within them.  make sure that both the ignorable and its
		//         content are omitted.

		this.ignores("reset");
		this.ignores("add-to", "*_ign");

		this.do_fsobj_mkdir("dir1a");
		this.do_fsobj_mkdir("dir1a/dir2a");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1a/dir2b");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b");
		this.do_fsobj_mkdir("dir1b/dir2a");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b/dir2b");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("foo1_ign");
		this.do_fsobj_mkdir("foo1_ign/bar2");
		this.do_fsobj_mkdir("dir1a/foo2_ign");
		this.do_fsobj_mkdir("dir1a/foo2_ign/bar3");
		this.do_fsobj_mkdir("dir1a/dir2a/foo3_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/foo3_ign/bar4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/foo4_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/foo4_ign/bar5");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/foo5_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/foo5_ign/bar6");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7");

		// verify that STATUS identifies ALL of our changes.

		var expect_all = new Array;
		expect_all["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1a/dir2b",
								"@/dir1a/dir2b/dir3",
								"@/dir1a/dir2b/dir3/dir4",
								"@/dir1a/dir2b/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5",
								"@/foo1_ign",
								"@/foo1_ign/bar2",
								"@/dir1a/foo2_ign",
								"@/dir1a/foo2_ign/bar3",
								"@/dir1a/dir2a/foo3_ign",
								"@/dir1a/dir2a/foo3_ign/bar4",
								"@/dir1a/dir2a/dir3/foo4_ign",
								"@/dir1a/dir2a/dir3/foo4_ign/bar5",
								"@/dir1a/dir2a/dir3/dir4/foo5_ign",
								"@/dir1a/dir2a/dir3/dir4/foo5_ign/bar6",
								"@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign",
								"@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7"
						  ];
		vscript_test_wc__confirm_wii(expect_all);

		// verify that a normal STATUS filters out the ignorables and 
		// the contents of the omitted directories.

		var expect_ign = new Array;
		expect_ign["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1a/dir2b",
								"@/dir1a/dir2b/dir3",
								"@/dir1a/dir2b/dir3/dir4",
								"@/dir1a/dir2b/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5"
						  ];
		expect_ign["Ignored"] = [ "@/foo1_ign",
								  // "@/foo1_ign/bar2",			// With the changes for W6508 we no longer
								  "@/dir1a/foo2_ign",
								  // "@/dir1a/foo2_ign/bar3",	// dive into ignored directories, so these
								  "@/dir1a/dir2a/foo3_ign",
								  // "@/dir1a/dir2a/foo3_ign/bar4",	// items won't show up.
								  "@/dir1a/dir2a/dir3/foo4_ign",
								  // "@/dir1a/dir2a/dir3/foo4_ign/bar5",
								  "@/dir1a/dir2a/dir3/dir4/foo5_ign",
								  // "@/dir1a/dir2a/dir3/dir4/foo5_ign/bar6",
								  "@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign",
								  // "@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7"
						  ];

		vscript_test_wc__confirm(expect_ign);

		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/foo5_ign/bar6");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/foo5_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/foo4_ign/bar5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/foo4_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/foo3_ign/bar4");
		this.do_fsobj_remove_dir("dir1a/dir2a/foo3_ign");
		this.do_fsobj_remove_dir("dir1a/foo2_ign/bar3");
		this.do_fsobj_remove_dir("dir1a/foo2_ign");
		this.do_fsobj_remove_dir("foo1_ign/bar2");
		this.do_fsobj_remove_dir("foo1_ign");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2b");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2a");
		this.do_fsobj_remove_dir("dir1b");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2b");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2a");
		this.do_fsobj_remove_dir("dir1a");

        var expect_nothing = new Array;
        vscript_test_wc__confirm_wii(expect_nothing);

		print("Reached bottom of " + 'test_status_3');
	}

	this.test_status_4 = function test_status_4()
	{
		// test 4: add a series of "ignorable directories and files"
		//         and make sure that the pattern can match both.

		this.ignores("reset");
		this.ignores("add-to", "*_ign");

		this.do_fsobj_mkdir("dir1a");
		this.do_fsobj_mkdir("dir1a/dir2a");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_create_file("file1.txt_ign");
		this.do_fsobj_create_file("dir1a/file2.txt_ign");
		this.do_fsobj_create_file("dir1a/dir2a/file3.txt_ign");
		this.do_fsobj_create_file("dir1a/dir2a/dir3/file4.txt_ign");
		this.do_fsobj_create_file("dir1a/dir2a/dir3/dir4/file5.txt_ign");
		this.do_fsobj_create_file("dir1a/dir2a/dir3/dir4/dir5/file6.txt_ign");
		this.do_fsobj_mkdir("foo1_ign");
		this.do_fsobj_mkdir("foo1_ign/bar2");
		this.do_fsobj_mkdir("dir1a/foo2_ign");
		this.do_fsobj_mkdir("dir1a/foo2_ign/bar3");
		this.do_fsobj_mkdir("dir1a/dir2a/foo3_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/foo3_ign/bar4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/foo4_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/foo4_ign/bar5");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/foo5_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/foo5_ign/bar6");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7");

		// verify that STATUS identifies ALL of our changes.

		var expect_all = new Array;
		expect_all["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/file1.txt_ign",
								"@/dir1a/file2.txt_ign",
								"@/dir1a/dir2a/file3.txt_ign",
								"@/dir1a/dir2a/dir3/file4.txt_ign",
								"@/dir1a/dir2a/dir3/dir4/file5.txt_ign",
								"@/dir1a/dir2a/dir3/dir4/dir5/file6.txt_ign",
								"@/foo1_ign",
								"@/foo1_ign/bar2",
								"@/dir1a/foo2_ign",
								"@/dir1a/foo2_ign/bar3",
								"@/dir1a/dir2a/foo3_ign",
								"@/dir1a/dir2a/foo3_ign/bar4",
								"@/dir1a/dir2a/dir3/foo4_ign",
								"@/dir1a/dir2a/dir3/foo4_ign/bar5",
								"@/dir1a/dir2a/dir3/dir4/foo5_ign",
								"@/dir1a/dir2a/dir3/dir4/foo5_ign/bar6",
								"@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign",
								"@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7"
						  ];
		vscript_test_wc__confirm_wii(expect_all);

		// verify that a normal STATUS filters out the ignorables and 
		// the contents of the omitted directories.

		var expect_ign = new Array;
		expect_ign["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5"
						  ];
		expect_ign["Ignored"] = [ "@/file1.txt_ign",
								  "@/dir1a/file2.txt_ign",
								  "@/dir1a/dir2a/file3.txt_ign",
								  "@/dir1a/dir2a/dir3/file4.txt_ign",
								  "@/dir1a/dir2a/dir3/dir4/file5.txt_ign",
								  "@/dir1a/dir2a/dir3/dir4/dir5/file6.txt_ign",
								  "@/foo1_ign",
								  // "@/foo1_ign/bar2",			// With the changes for W6508 we no longer
								  "@/dir1a/foo2_ign",
								  // "@/dir1a/foo2_ign/bar3",	// dive into ignored directories, so these
								  "@/dir1a/dir2a/foo3_ign",
								  // "@/dir1a/dir2a/foo3_ign/bar4",	// items won't show up.
								  "@/dir1a/dir2a/dir3/foo4_ign",
								  // "@/dir1a/dir2a/dir3/foo4_ign/bar5",
								  "@/dir1a/dir2a/dir3/dir4/foo5_ign",
								  // "@/dir1a/dir2a/dir3/dir4/foo5_ign/bar6",
								  "@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign",
								  // "@/dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7"
						  ];

		vscript_test_wc__confirm(expect_ign);

		this.do_fsobj_remove_file("dir1a/dir2a/dir3/dir4/dir5/file6.txt_ign");
		this.do_fsobj_remove_file("dir1a/dir2a/dir3/dir4/file5.txt_ign");
		this.do_fsobj_remove_file("dir1a/dir2a/dir3/file4.txt_ign");
		this.do_fsobj_remove_file("dir1a/dir2a/file3.txt_ign");
		this.do_fsobj_remove_file("dir1a/file2.txt_ign");
		this.do_fsobj_remove_file("file1.txt_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign/bar7");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5/foo6_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/foo5_ign/bar6");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/foo5_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/foo4_ign/bar5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/foo4_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/foo3_ign/bar4");
		this.do_fsobj_remove_dir("dir1a/dir2a/foo3_ign");
		this.do_fsobj_remove_dir("dir1a/foo2_ign/bar3");
		this.do_fsobj_remove_dir("dir1a/foo2_ign");
		this.do_fsobj_remove_dir("foo1_ign/bar2");
		this.do_fsobj_remove_dir("foo1_ign");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2a");
		this.do_fsobj_remove_dir("dir1a");

        var expect_nothing = new Array;
        vscript_test_wc__confirm_wii(expect_nothing);

		print("Reached bottom of " + 'test_status_4');
	}

	//////////////////////////////////////////////////////////////////

	this.test_status_5 = function test_status_5()
	{
		// test 5: baseline; no ignores.  make sure explicitly named DIRECTORY items work.

		this.ignores("reset");

		this.do_fsobj_mkdir("dir1a");
		this.do_fsobj_mkdir("dir1a/dir2a");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1a/dir2b");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b");
		this.do_fsobj_mkdir("dir1b/dir2a");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b/dir2b");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1c");

		// verify that STATUS identifies ALL of our changes.

		var expect_all = new Array;
		expect_all["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1a/dir2b",
								"@/dir1a/dir2b/dir3",
								"@/dir1a/dir2b/dir3/dir4",
								"@/dir1a/dir2b/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5",
								"@/dir1c"
							  ];
		vscript_test_wc__confirm_wii(expect_all);

		// verify that STATUS with only an EXPLICITLY NAMED list of items work.

		var status_exc = sg.wc.status({"src":["dir1a/dir2a",
						      "dir1b"]});
		var expect_exc = new Array;
		expect_exc["Found"] = [ "@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5"
							  ];
		vscript_test_wc__confirm(expect_exc, status_exc);

		this.do_fsobj_remove_dir("dir1c");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2b");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2a");
		this.do_fsobj_remove_dir("dir1b");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2b");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2a");
		this.do_fsobj_remove_dir("dir1a");

        var expect_nothing = new Array;
        vscript_test_wc__confirm_wii(expect_nothing);

		print("Reached bottom of " + 'test_status_5');
	}

	//////////////////////////////////////////////////////////////////

	this.test_status_6 = function test_status_6()
	{
		// test 6: baseline; no ignores.  make sure explicitly named FILE items work.

		this.ignores("reset");

		this.do_fsobj_mkdir("dir1a");
		this.do_fsobj_mkdir("dir1a/dir2a");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1a/dir2b");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b");
		this.do_fsobj_mkdir("dir1b/dir2a");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1b/dir2b");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_mkdir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_mkdir("dir1c");

		this.do_fsobj_create_file("file1.txt");
		this.do_fsobj_create_file("dir1a/file2.txt");
		this.do_fsobj_create_file("dir1a/dir2a/file3.txt");
		this.do_fsobj_create_file("dir1a/dir2a/dir3/file4.txt");
		this.do_fsobj_create_file("dir1a/dir2a/dir3/dir4/file5.txt");
		this.do_fsobj_create_file("dir1a/dir2a/dir3/dir4/dir5/file6.txt");

		// verify that STATUS identifies ALL of our changes.

		var expect_all = new Array;
		expect_all["Found"] = [ "@/dir1a",
								"@/dir1a/dir2a",
								"@/dir1a/dir2a/dir3",
								"@/dir1a/dir2a/dir3/dir4",
								"@/dir1a/dir2a/dir3/dir4/dir5",
								"@/dir1a/dir2b",
								"@/dir1a/dir2b/dir3",
								"@/dir1a/dir2b/dir3/dir4",
								"@/dir1a/dir2b/dir3/dir4/dir5",
								"@/dir1b",
								"@/dir1b/dir2a",
								"@/dir1b/dir2a/dir3",
								"@/dir1b/dir2a/dir3/dir4",
								"@/dir1b/dir2a/dir3/dir4/dir5",
								"@/dir1b/dir2b",
								"@/dir1b/dir2b/dir3",
								"@/dir1b/dir2b/dir3/dir4",
								"@/dir1b/dir2b/dir3/dir4/dir5",
								"@/dir1c",
								"@/file1.txt",
								"@/dir1a/file2.txt",
								"@/dir1a/dir2a/file3.txt",
								"@/dir1a/dir2a/dir3/file4.txt",
								"@/dir1a/dir2a/dir3/dir4/file5.txt",
								"@/dir1a/dir2a/dir3/dir4/dir5/file6.txt"
							  ];
		vscript_test_wc__confirm_wii(expect_all);

		// verify that STATUS with only an EXPLICITLY NAMED list of items work.

		var status_exc = sg.wc.status({"src":["file1.txt", 
											  "dir1a/file2.txt",
											  "dir1a/dir2a/file3.txt"]});
		var expect_exc = new Array;
		expect_exc["Found"] = [ "@/file1.txt",
								"@/dir1a/file2.txt",
								"@/dir1a/dir2a/file3.txt"
							  ];
		vscript_test_wc__confirm(expect_exc, status_exc);

		this.do_fsobj_remove_file("dir1a/dir2a/dir3/dir4/dir5/file6.txt");
		this.do_fsobj_remove_file("dir1a/dir2a/dir3/dir4/file5.txt");
		this.do_fsobj_remove_file("dir1a/dir2a/dir3/file4.txt");
		this.do_fsobj_remove_file("dir1a/dir2a/file3.txt");
		this.do_fsobj_remove_file("dir1a/file2.txt");
		this.do_fsobj_remove_file("file1.txt");

		this.do_fsobj_remove_dir("dir1c");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2b");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1b/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1b/dir2a");
		this.do_fsobj_remove_dir("dir1b");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2b/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2b");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4/dir5");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3/dir4");
		this.do_fsobj_remove_dir("dir1a/dir2a/dir3");
		this.do_fsobj_remove_dir("dir1a/dir2a");
		this.do_fsobj_remove_dir("dir1a");

        var expect_nothing = new Array;
        vscript_test_wc__confirm_wii(expect_nothing);

		print("Reached bottom of " + 'test_status_6');
	}

}
function st_Status_BuildAll() 
{
	//This function builds one of each type of status  It takes some doing.
	this.testBuildAllStatuses = function()
	{
		sg.file.write("modifyMe.txt", "content");
		sg.file.write("loseMe.txt", "content");
		sg.file.write("renameMe.txt", "content");
		sg.file.write("renameAndModifyMe.txt", "content");
		sg.file.write("moveMe.txt", "content");
		sg.file.write("modifyMe.txt", "content");
		sg.file.write("deleteMe.txt", "content");

		sg.file.write("namespaceConflict.txt", "content");
		sg.file.write("contentConflict.txt", "content");
		sg.file.write("namespaceAndContentConflict.txt", "content");
		sg.file.write("autoMergable.txt", "line 1\r\nline 2\r\nline 3\r\nline 4\r\nline 5\r\nline 6\r\nline 7\r\nline 8\r\n");

		sg.fs.mkdir("moveTarget1");
		sg.fs.mkdir("moveTarget2");

		vscript_test_wc__addremove();

		ancestorCommit = vscript_test_wc__commit();
		//In the first child commit, mess create conflicts.
		sg.file.write("contentConflict.txt", "new content on that line");
		sg.file.write("namespaceAndContentConflict.txt", "new content on that line");
		vscript_test_wc__move("namespaceAndContentConflict.txt", "moveTarget1");
		vscript_test_wc__move("namespaceConflict.txt", "moveTarget1");
		sg.file.write("autoMergable.txt", "line 1 changed\r\nline 2\r\nline 3\r\nline 4\r\nline 5\r\nline 6\r\nline 7\r\nline 8\r\n");
		vscript_test_wc__commit();

		//now update back to the ancestor
		vscript_test_wc__update( ancestorCommit );
		sg.exec(vv, "branch", "attach", "master");

		sg.file.write("contentConflict.txt", "conflicting content on that line");
		sg.file.write("namespaceAndContentConflict.txt", "conflicting content on that line");
		vscript_test_wc__move("namespaceAndContentConflict.txt", "moveTarget2");
		vscript_test_wc__move("namespaceConflict.txt", "moveTarget2");
		sg.file.write("autoMergable.txt", "line 1\r\nline 2\r\nline 3\r\nline 4\r\nline 5\r\nline 6\r\nline 7\r\nline 8 changed\r\n");
		vscript_test_wc__commit();
		
		testlib.execVV("merge");

		sg.file.write("addMe.txt", "content");
		vscript_test_wc__add("addMe.txt");
		vscript_test_wc__move("moveMe.txt", "moveTarget1");
		vscript_test_wc__rename("renameMe.txt", "newName.txt");
		vscript_test_wc__rename("renameAndModifyMe.txt", "iveBeenRenamedAndModified.txt");
		vscript_test_wc__remove("deleteMe.txt");
		sg.file.write("findMe.txt", "content");
		sg.file.write("iveBeenRenamedAndModified.txt", "new content");
		sg.file.write("modifyMe.txt", "new content");
		sg.fs.remove("loseMe.txt");
	}
}
function st_Status_AfterMove() 
{
	//This function builds one of each type of status  It takes some doing.
	this.testAfterMove = function()
	{

		sg.fs.mkdir_recursive("folder/sub/sub2/moveTarget1");
		sg.file.write("folder/sub/sub2/moveTarget1/subFile.txt", "content");
		sg.fs.mkdir("moveTarget2");
		sg.file.write("moveTarget2/subFile.txt", "content");
		vscript_test_wc__addremove();

		vscript_test_wc__commit();
		vscript_test_wc__move("folder/sub/sub2/moveTarget1", "moveTarget2");
		testlib.execVV("status", "moveTarget2");
	}
}

function st_Status_CommitWithLost() 
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

	//This function builds one of each type of status  It takes some doing.
	this.testCommitWithLost = function()
	{

		sg.file.write("editedfile.txt", "hello world");
		sg.file.write("subFile.txt", "content");
		vscript_test_wc__addremove();

		vscript_test_wc__commit();
		sg.fs.remove("subFile.txt");

		sg.file.write("otherFile.txt", "content");
		vscript_test_wc__add("otherFile.txt");
		this.do_fsobj_append_file("editedfile.txt", "goodbye cruel world");

		var expect_test = new Array;
		expect_test["Added"] = [ "@/otherFile.txt" ];
		expect_test["Lost"]  = [ "@/subFile.txt" ];
		expect_test["Modified"] = [ "@/editedfile.txt" ];
		vscript_test_wc__confirm_wii(expect_test);

		// we used to be able to a commit when there were LOST items.
		// i fixed that for W7399 to throw if the LOST item was to
		// be commited.  (which broke this test.)

		var expect = "Cannot commit a LOST entry";
		try
		{
			vscript_test_wc__commit();
			testlib.ok( (false), "Commit should have thrown error: '" + expect + "'" );
		}
		catch (e)
		{
			this.my_verify_expected_throw(e, expect);
		}

		// try to do a partial commit of JUST the modified file.
		// (which should work since we are not trying to commit
		// the lost file.)

		vscript_test_wc__commit_np( { "src" : "editedfile.txt",
					      "message" : "Partial commit of otherFile.txt" } );

		var expect_test = new Array;
		expect_test["Added"] = [ "@/otherFile.txt" ];
		expect_test["Lost"]  = [ "@/subFile.txt" ];
		vscript_test_wc__confirm_wii(expect_test);

		// try to do a partial commit of JUST the added file.
		// (which should work since we are not trying to commit
		// the lost file.)

		vscript_test_wc__commit_np( { "src" : "otherFile.txt",
					      "message" : "Partial commit of otherFile.txt" } );

		var expect_test = new Array;
		expect_test["Lost"]  = [ "@/subFile.txt" ];
		vscript_test_wc__confirm_wii(expect_test);
	}
}
