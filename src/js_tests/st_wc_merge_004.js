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

function st_wc_merge_004()
{
    var my_group = "st_wc_merge_004";	// this variable must match the above group name.

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
	vscript_test_wc__rename("dir_A", "B__dir_A");
	vscript_test_wc__rename("dir_B", "B__dir_B");
	vscript_test_wc__rename("dir_C", "B__dir_C");
	vscript_test_wc__rename("dir_D", "B__dir_D");
	vscript_test_wc__rename("dir_E", "B__dir_E");
	vscript_test_wc__move("dir_F", "dir_M");
	vscript_test_wc__move("dir_G", "dir_M");
	vscript_test_wc__move("dir_H", "dir_M");
	vscript_test_wc__move("dir_I", "dir_M");
	vscript_test_wc__move("dir_J", "dir_M");
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__move("dir_A", "dir_M");
	vscript_test_wc__move("dir_B", "dir_M");
	vscript_test_wc__move("dir_C", "dir_M");
	vscript_test_wc__move("dir_D", "dir_M");
	vscript_test_wc__move("dir_E", "dir_M");
	vscript_test_wc__rename("dir_F", "C__dir_F");
	vscript_test_wc__rename("dir_G", "C__dir_G");
	vscript_test_wc__rename("dir_H", "C__dir_H");
	vscript_test_wc__rename("dir_I", "C__dir_I");
	vscript_test_wc__rename("dir_J", "C__dir_J");
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

	// each changed entry was only changed in orthogonal
	// ways in each cset, so we will exercise the
	// "A-vs-L0 present *and* A-vs-L1 present
	// code without conflict.
	//
	// with or without a WC change.

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC==B,WC!=C)"] = [ "@/dir_M/B__dir_A",
						       "@/dir_M/B__dir_B",
						       "@/dir_M/B__dir_C",
						       "@/dir_M/B__dir_D",
						       "@/dir_M/B__dir_E",
						     ];
	expect_test["Renamed (WC!=A,WC!=B,WC==C)"] = [ "@/dir_M/C__dir_F",
						       "@/dir_M/C__dir_G",
						       "@/dir_M/C__dir_H",
						       "@/dir_M/C__dir_I",
						       "@/dir_M/C__dir_J",
						     ];
	expect_test["Moved (WC!=A,WC!=B,WC==C)"] = [ "@/dir_M/B__dir_A",
						     "@/dir_M/B__dir_B",
						     "@/dir_M/B__dir_C",
						     "@/dir_M/B__dir_D",
						     "@/dir_M/B__dir_E",
						   ];
	expect_test["Moved (WC!=A,WC==B,WC!=C)"] = [ "@/dir_M/C__dir_F",
						     "@/dir_M/C__dir_G",
						     "@/dir_M/C__dir_H",
						     "@/dir_M/C__dir_I",
						     "@/dir_M/C__dir_J",
						   ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	// do some post-merge renames to dirty things up.

	vscript_test_wc__rename("dir_M/B__dir_A", "BW__dir_A"); // rename+rename
	vscript_test_wc__rename("dir_M/C__dir_F", "CW__dir_F");

	vscript_test_wc__rename("dir_M/B__dir_B", "dir_B"); // undo
	vscript_test_wc__rename("dir_M/C__dir_G", "dir_G");

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC==B,WC!=C)"] = [ "@/dir_M/B__dir_C",
						       "@/dir_M/B__dir_D",
						       "@/dir_M/B__dir_E",
						     ];
	expect_test["Renamed (WC!=A,WC!=B,WC==C)"] = [ "@/dir_M/C__dir_H",
						       "@/dir_M/C__dir_I",
						       "@/dir_M/C__dir_J",
						     ];
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"] = [ "@/dir_M/BW__dir_A",
						       "@/dir_M/CW__dir_F",
						     ];
	expect_test["Renamed (WC==A,WC!=B,WC==C)"] = [ "@/dir_M/dir_B",
						     ];
	expect_test["Renamed (WC==A,WC==B,WC!=C)"] = [ "@/dir_M/dir_G",
						     ];
	expect_test["Moved (WC!=A,WC!=B,WC==C)"] = [ "@/dir_M/BW__dir_A",
						     "@/dir_M/dir_B",
						     "@/dir_M/B__dir_C",
						     "@/dir_M/B__dir_D",
						     "@/dir_M/B__dir_E",
						   ];
	expect_test["Moved (WC!=A,WC==B,WC!=C)"] = [ "@/dir_M/CW__dir_F",
						     "@/dir_M/dir_G",
						     "@/dir_M/C__dir_H",
						     "@/dir_M/C__dir_I",
						     "@/dir_M/C__dir_J",
						   ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);
    }

}
