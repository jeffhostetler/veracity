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
// Basic tests that DO NOT require UPDATE nor REVERT.
//////////////////////////////////////////////////////////////////

function st_wc_port_P3416()
{
    var my_group = "st_wc_port_P3416";	// this variable must match the above group name.

    this.no_setup = true;		// do not create an initial REPO and WD.

    if (sg.platform() != "LINUX")
    {
	print("Skipping test on non-Linux system -- need fully case sensitive file system.");
	return;
    }

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
	this.SG_ERR_WC_PORT_FLAGS = "Portability conflict";	// see sg_error.c

	var unique = my_group + "_" + new Date().getTime();

	var repoName = unique;
	var rootDir = pathCombine(tempDir, unique);
	var wdDirs = [];
	var csets = {};
	var nrWD = 0;

	var my_make_wd = function(obj)
	{
	    var new_wd = pathCombine(rootDir, "wd_" + nrWD);

	    vscript_test_wc__print_section_divider("Creating WD[" + nrWD + "]: " + new_wd);

	    obj.do_fsobj_mkdir_recursive( new_wd );
	    obj.do_fsobj_cd( new_wd );
	    wdDirs.push( new_wd );
	    
	    nrWD += 1;

	    return new_wd;
	}

	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	my_make_wd(this);
	sg.vv2.init_new_repo( { "repo" : repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(repoName);

	//////////////////////////////////////////////////////////////////
	// Build a sequence of CSETs using current WD.

	this.do_fsobj_mkdir("dir_A");
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	// Create 2 files that are unique/distinct on a normal
	// Linux filesystem, but that will collide on a Windows
	// filesystem.

	vscript_test_wc__write_file("foo", "Hello World!\n");
	vscript_test_wc__write_file("FOO", "Goodbye cruel World!\n");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/foo", "@/FOO" ];
        vscript_test_wc__confirm_wii(expect_test);

	// With the port-mask turned on fully,
	// we should get a portability error on
	// the second if we try to add them both.
	// We should not get an error for the first.

	vscript_test_wc__set_debug_port_mask__on();
	vscript_test_wc__add( "foo" );

	var expect_test = new Array;
	expect_test["Added"] = [ "@/foo" ];
	expect_test["Found"] = [ "@/FOO" ];
        vscript_test_wc__confirm_wii(expect_test);

	var fnstr = formatFunctionString("vscript_test_wc__add", [ "FOO" ]);
	testlib.verifyFailure(fnstr,
			      "Portability during ADD",
			      "Portability conflict");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/foo" ];
	expect_test["Found"] = [ "@/FOO" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// The second example in P3416 asks about RENAME
	// with the target potentially colliding with dirt.
	if (0)
	{
	    // This part is disabled until W6226 is addressed.

	    vscript_test_wc__print_section_divider("Try rename...");

	    vscript_test_wc__rename("foo", "bar");	// move away from conflicting name

	    var expect_test = new Array;
	    expect_test["Added"] = [ "@/bar" ];
	    expect_test["Found"] = [ "@/FOO" ];
            vscript_test_wc__confirm_wii(expect_test);

	    vscript_test_wc__rename("bar", "Foo");	// this should not complain

	    var expect_test = new Array;
	    expect_test["Added"] = [ "@/Foo" ];
	    expect_test["Found"] = [ "@/FOO" ];
            vscript_test_wc__confirm_wii(expect_test);
	}
    }
}
