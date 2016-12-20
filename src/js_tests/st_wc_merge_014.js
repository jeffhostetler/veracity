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

function st_wc_merge_014()
{
    var my_group = "st_wc_merge_014";	// this variable must match the above group name.

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
	this.do_fsobj_mkdir("dir_B");
	this.do_fsobj_mkdir("dir_B/dir_BB");
	this.do_fsobj_mkdir("dir_A/dir_AA/dir_BBB");
	vscript_test_wc__addremove();
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_C");
	this.do_fsobj_mkdir("dir_C/dir_CC");
	this.do_fsobj_mkdir("dir_A/dir_AA/dir_CCC");
	vscript_test_wc__addremove();
	csets.C = vscript_test_wc__commit("C");

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout B, create some dirt, and merge C into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.B
				      } );
	var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	// make some pre-merge dirt.
	// this dirt DOES collide with stuff in B and C,
	// so MERGE might freak out until I get the dirty
	// merge code added back.

	vscript_test_wc__remove_dir("dir_A/dir_AA/dir_BBB");
	vscript_test_wc__remove_dir("dir_A/dir_AA");
	vscript_test_wc__rename("dir_A", "dirty__dir_A");

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/dirty__dir_A"
				 ];
	expect_test["Removed"] = [ "@b/dir_A/dir_AA/dir_BBB",
				   "@b/dir_A/dir_AA",
				 ];
        vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__merge_np__expect_error( { "rev" : csets.C }, "This type of merge requires a clean working copy" );
	vscript_test_wc__merge_np( { "rev" : csets.C, "allow_dirty" : true } );

	var expect_test = new Array;
	expect_test["Added (Merge)"]                 = [ "@/dir_C", 
							 "@/dir_C/dir_CC",
							 "@/dirty__dir_A/dir_AA/dir_CCC" ];
	expect_test["Renamed"]                       = [ "@/dirty__dir_A" ];
	expect_test["Removed"]                       = [ "@b/dir_A/dir_AA/dir_BBB" ];
	expect_test["Unresolved"]                    = [ "@/dirty__dir_A/dir_AA" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/dirty__dir_A/dir_AA" ];
        vscript_test_wc__confirm_wii(expect_test);

    }

}
