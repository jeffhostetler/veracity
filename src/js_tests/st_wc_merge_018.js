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
// Basic MERGE tests that DO NOT require UPDATE nor REVERT.
//////////////////////////////////////////////////////////////////

function st_wc_merge_018()
{
    var my_group = "st_wc_merge_018";	// this variable must match the above group name.

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
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
	this.do_fsobj_mkdir("dir_A/dir_AA");
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_X");
	vscript_test_wc__addremove();
	vscript_test_wc__remove_dir("dir_A/dir_AA");
	csets.X = vscript_test_wc__commit("X");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_Y");
	vscript_test_wc__addremove();
	csets.Y = vscript_test_wc__commit("Y");

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout X, and merge Y into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.X
				      } );
	var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);


	vscript_test_wc__merge_np( { "rev" : csets.Y } );


	var expect_test = new Array;
	expect_test["Added (C)"]      = [ "@/dir_Y" ];
	expect_test["Added (B)"]      = [ "@/dir_X" ];
	expect_test["Removed (B,WC)"] = [ "@c/dir_A/dir_AA" ];

        vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout Y, and merge X into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.Y
				      } );
	var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);


	vscript_test_wc__merge_np( { "rev" : csets.X } );


	var expect_test = new Array;
	expect_test["Added (C)"]      = [ "@/dir_X" ];
	expect_test["Added (B)"]      = [ "@/dir_Y" ];
	expect_test["Removed (C,WC)"] = [ "@b/dir_A/dir_AA" ];

        vscript_test_wc__mstatus_confirm_wii(expect_test);

    }

}
