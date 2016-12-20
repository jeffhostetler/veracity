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

function st_wc_port_000()
{
    var my_group = "st_wc_port_000";	// this variable must match the above group name.

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

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

	if (sg.platform() != "WINDOWS")
	{
	    // Create a directory name that the default portability
	    // code will complain about.  Make sure ADD and ADDREMOVE
	    // do complain (and in the same way).
	    //
	    // This test doesn't work on Windows because we call
	    // GetFullPathNameW() on the args and it strips out
	    // final dots/spaces (while is it flattening .. references).

	    this.do_fsobj_mkdir("dir_A/dir_AA.");	// FINAL_DOT_SPACE

	    var fnstr = formatFunctionString("vscript_test_wc__add", [ "dir_A/dir_AA." ]);
	    testlib.verifyFailure(fnstr, "Portability during ADD", this.SG_ERR_WC_PORT_FLAGS);

	    var fnstr = formatFunctionString("vscript_test_wc__addremove", [ ]);
	    testlib.verifyFailure(fnstr, "Portability during ADDREMOVE", this.SG_ERR_WC_PORT_FLAGS);
	}

	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_xxx");
	vscript_test_wc__addremove();
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_XXX");
	vscript_test_wc__addremove();
	csets.C = vscript_test_wc__commit("C");

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout B and merge C into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.B
				      } );

	// MERGE does not throw SG_ERR_WC_PORT_FLAGS.
	// Rather, it converts everything into unresolved
	// issues.

	vscript_test_wc__merge_np( { "rev" : csets.C } );

	var expect_test = new Array;
	expect_test["Renamed"]                       = [ "@/dir_xxx~g[0-9a-f]*" ];
	expect_test["Added (Merge)" ]                = [ "@/dir_XXX~g[0-9a-f]*" ];
	expect_test["Unresolved"]                    = [ "@/dir_xxx~g[0-9a-f]*", "@/dir_XXX~g[0-9a-f]*" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/dir_xxx~g[0-9a-f]*", "@/dir_XXX~g[0-9a-f]*" ];

        vscript_test_wc__confirm_wii(expect_test);

	var expect_test = new Array;
	expect_test["Added (B)"]                     = [ "@/dir_xxx~g[0-9a-f]*" ];
	expect_test["Added (C)" ]                    = [ "@/dir_XXX~g[0-9a-f]*" ];
	expect_test["Renamed (WC)"]                  = [ "@/dir_xxx~g[0-9a-f]*", "@/dir_XXX~g[0-9a-f]*" ];
	expect_test["Unresolved"]                    = [ "@/dir_xxx~g[0-9a-f]*", "@/dir_XXX~g[0-9a-f]*" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/dir_xxx~g[0-9a-f]*", "@/dir_XXX~g[0-9a-f]*" ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

    }
}
