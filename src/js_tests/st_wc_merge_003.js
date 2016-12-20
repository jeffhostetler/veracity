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

function st_wc_merge_003()
{
    var my_group = "st_wc_merge_003";	// this variable must match the above group name.

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
	this.do_fsobj_mkdir("dir_B");
	this.do_fsobj_mkdir("dir_C");
	this.do_fsobj_mkdir("dir_D");
	this.do_fsobj_mkdir("dir_E");
	this.do_fsobj_mkdir("dir_F");
	this.do_fsobj_mkdir("dir_G");
	this.do_fsobj_mkdir("dir_H");
	this.do_fsobj_mkdir("dir_I");
	this.do_fsobj_mkdir("dir_J");
	this.do_fsobj_mkdir("dir_K");
	this.do_fsobj_mkdir("dir_L");
	this.do_fsobj_mkdir("dir_M");

	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__rename("dir_A", "BC__dir_A");
	vscript_test_wc__rename("dir_B", "BC__dir_B");
	vscript_test_wc__rename("dir_C", "BC__dir_C");
	vscript_test_wc__rename("dir_D", "BC__dir_D");
	vscript_test_wc__rename("dir_E", "BC__dir_E");
	vscript_test_wc__rename("dir_F", "BC__dir_F");
	vscript_test_wc__rename("dir_G", "BC__dir_G");
	vscript_test_wc__rename("dir_H", "BC__dir_H");
	vscript_test_wc__rename("dir_I", "B__dir_I");	// odd one to ensure unique csets
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__rename("dir_A", "BC__dir_A");
	vscript_test_wc__rename("dir_B", "BC__dir_B");
	vscript_test_wc__rename("dir_C", "BC__dir_C");
	vscript_test_wc__rename("dir_D", "BC__dir_D");
	vscript_test_wc__rename("dir_E", "BC__dir_E");
	vscript_test_wc__rename("dir_F", "BC__dir_F");
	vscript_test_wc__rename("dir_G", "BC__dir_G");
	vscript_test_wc__rename("dir_H", "BC__dir_H");
	vscript_test_wc__rename("dir_J", "C__dir_J");	// odd one to ensure unique csets
	csets.C = vscript_test_wc__commit("C");

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//
	// (We don't yet have UPDATE and REVERT working in
	// the WC version of the code.)
	//////////////////////////////////////////////////////////////////

	vscript_test_wc__print_section_divider("Checkout B and merge C into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.B
				      } );
	vscript_test_wc__merge_np( { "rev" : csets.C } );

	// each BC__ entry was changed the same way in both of
	// the above csets, therefore we will exercise the code
	// for A-vs-L0 present and A-vs-L1 present with or
	// without a WC change.

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC==B,WC==C)"] = [ "@/BC__dir_A",
						       "@/BC__dir_B",
						       "@/BC__dir_C",
						       "@/BC__dir_D",
						       "@/BC__dir_E",
						       "@/BC__dir_F",
						       "@/BC__dir_G",
						       "@/BC__dir_H",
				      ];
	expect_test["Renamed (WC!=A,WC==B,WC!=C)"] = [ "@/B__dir_I",
						     ];
	expect_test["Renamed (WC!=A,WC!=B,WC==C)"] = [ "@/C__dir_J",
						     ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	// do some post-merge renames to dirty things up.

	vscript_test_wc__rename("BC__dir_A", "BCW__dir_A"); // rename+rename

	vscript_test_wc__rename("BC__dir_B", "dir_B"); // undo

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC==B,WC==C)"] = [ "@/BC__dir_C",
						       "@/BC__dir_D",
						       "@/BC__dir_E",
						       "@/BC__dir_F",
						       "@/BC__dir_G",
						       "@/BC__dir_H",
				      ];
	expect_test["Renamed (WC!=A,WC==B,WC!=C)"] = [ "@/B__dir_I",
						     ];
	expect_test["Renamed (WC!=A,WC!=B,WC==C)"] = [ "@/C__dir_J",
						     ];
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"] = [ "@/BCW__dir_A"
						     ];
	expect_test["Renamed (WC==A,WC!=B,WC!=C)"] = [ "@/dir_B"
						     ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

    }

}
