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

function st_wc_merge_009()
{
    var my_group = "st_wc_merge_009";	// this variable must match the above group name.

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
	this.do_fsobj_mkdir("dir_A/dir_AA/dir_AAA");
	this.do_fsobj_mkdir("dir_A/dir_AA/dir_AAA/dir_AAAA");
	// TODO create some files too.
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
	this.do_fsobj_mkdir("dir_B/dir_BB/dir_BBB");
	this.do_fsobj_mkdir("dir_B/dir_BB/dir_BBB/dir_BBBB");
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
	this.do_fsobj_mkdir("dir_C/dir_CC/dir_CCC");
	this.do_fsobj_mkdir("dir_C/dir_CC/dir_CCC/dir_CCCC");
	vscript_test_wc__addremove();
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

	// use regular STATUS to check results.

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/dir_C",
					 "@/dir_C/dir_CC",
					 "@/dir_C/dir_CC/dir_CCC",
					 "@/dir_C/dir_CC/dir_CCC/dir_CCCC"
				       ];
        vscript_test_wc__confirm_wii(expect_test);

	// use MSTATUS to check results.

	var expect_test = new Array;
	expect_test["Added (C)"] = [ "@/dir_C",
				     "@/dir_C/dir_CC",
				     "@/dir_C/dir_CC/dir_CCC",
				     "@/dir_C/dir_CC/dir_CCC/dir_CCCC"
				   ];
	expect_test["Added (B)"] = [ "@/dir_B",
				     "@/dir_B/dir_BB",
				     "@/dir_B/dir_BB/dir_BBB",
				     "@/dir_B/dir_BB/dir_BBB/dir_BBBB"
				   ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	// commit the merge and do historical MSTATUS.

	csets.M1 = vscript_test_wc__commit("M1");

	var expect_test = new Array;
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout C and merge B into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.C
				      } );
	vscript_test_wc__merge_np( { "rev" : csets.B } );

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/dir_B",
					 "@/dir_B/dir_BB",
					 "@/dir_B/dir_BB/dir_BBB",
					 "@/dir_B/dir_BB/dir_BBB/dir_BBBB"
				       ];
        vscript_test_wc__confirm_wii(expect_test);
	
    }

}
