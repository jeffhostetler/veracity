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

function st_wc_merge_resolve_005m()
{
    var my_group = "st_wc_merge_resolve_005m";	// this variable must match the above group name.

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
	vscript_test_wc__move("dir_A", "dir_M");
	vscript_test_wc__move("dir_B", "dir_M");
	vscript_test_wc__move("dir_C", "dir_M");
	vscript_test_wc__move("dir_D", "dir_M");
	vscript_test_wc__move("dir_E", "dir_M");
	vscript_test_wc__remove("dir_F");
	vscript_test_wc__remove("dir_G");
	vscript_test_wc__remove("dir_H");
	vscript_test_wc__remove("dir_I");
	vscript_test_wc__remove("dir_J");
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__remove("dir_A");
	vscript_test_wc__remove("dir_B");
	vscript_test_wc__remove("dir_C");
	vscript_test_wc__remove("dir_D");
	vscript_test_wc__remove("dir_E");
	vscript_test_wc__move("dir_F", "dir_L");
	vscript_test_wc__move("dir_G", "dir_L");
	vscript_test_wc__move("dir_H", "dir_L");
	vscript_test_wc__move("dir_I", "dir_L");
	vscript_test_wc__move("dir_J", "dir_L");
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
	vscript_test_wc__merge_np( { "rev" : csets.C } );

	// DELETE-vs-MOVE conflict
	// each MOVE on one side conflicts with a DELETE on the other side.
	// MERGE will also do a merge-to-temp until resolved.

	var expect_test = new Array;
	expect_test["Moved (B!=A,WC==B)"] = [ "@/dir_M/dir_A",
					      "@/dir_M/dir_B",
					      "@/dir_M/dir_C",
					      "@/dir_M/dir_D",
					      "@/dir_M/dir_E",
					    ];

	expect_test["Moved (C!=A,WC==C)"] = [ "@/dir_L/dir_F",
					      "@/dir_L/dir_G",
					      "@/dir_L/dir_H",
					      "@/dir_L/dir_I",
					      "@/dir_L/dir_J",
					    ];

	expect_test["Existence (A,B,!C,WC)"] = [ "@/dir_M/dir_A",
						 "@/dir_M/dir_B",
						 "@/dir_M/dir_C",
						 "@/dir_M/dir_D",
						 "@/dir_M/dir_E",
					       ];
	expect_test["Existence (A,!B,C,WC)"] = [ "@/dir_L/dir_F",
						 "@/dir_L/dir_G",
						 "@/dir_L/dir_H",
						 "@/dir_L/dir_I",
						 "@/dir_L/dir_J",
					       ];
	expect_test["Unresolved"] = [ "@/dir_M/dir_A",
				      "@/dir_M/dir_B",
				      "@/dir_M/dir_C",
				      "@/dir_M/dir_D",
				      "@/dir_M/dir_E",
				      "@/dir_L/dir_F",
				      "@/dir_L/dir_G",
				      "@/dir_L/dir_H",
				      "@/dir_L/dir_I",
				      "@/dir_L/dir_J",
				    ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/dir_M/dir_A",
							 "@/dir_M/dir_B",
							 "@/dir_M/dir_C",
							 "@/dir_M/dir_D",
							 "@/dir_M/dir_E",
							 "@/dir_L/dir_F",
							 "@/dir_L/dir_G",
							 "@/dir_L/dir_H",
							 "@/dir_L/dir_I",
							 "@/dir_L/dir_J",
						       ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Before Resolve:");
	vscript_test_wc__resolve__list_all();

	var nrResolve = 0;
	//////////////////////////////////////////////////////////////////
	// Now let RESOLVE do something.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Resolve " + nrResolve++);

	// the above test did various delete-vs-delete and our
	// choice refers to 'existence' only.  EVEN IF WE ACCEPT
	// THE ANCESTOR (EXISTENCE) WE STILL GET THE MOVE.

	vscript_test_wc__resolve__accept("baseline", "@b/dir_M/dir_A");	// keep
	vscript_test_wc__resolve__accept("working",  "@b/dir_M/dir_B");	// keep
	vscript_test_wc__resolve__accept("ancestor", "@b/dir_M/dir_C");	// keep
	vscript_test_wc__resolve__accept("other",    "@b/dir_M/dir_D");	// remove
	vscript_test_wc__resolve__accept("other",    "@b/dir_M/dir_E");	// remove

	vscript_test_wc__resolve__accept("baseline", "@c/dir_L/dir_F");	// remove
	vscript_test_wc__resolve__accept("baseline", "@c/dir_L/dir_G");	// remove
	vscript_test_wc__resolve__accept("ancestor", "@c/dir_L/dir_H");	// keep
	vscript_test_wc__resolve__accept("other",    "@c/dir_L/dir_I");	// keep
	vscript_test_wc__resolve__accept("working",  "@c/dir_L/dir_J");	// keep

	vscript_test_wc__resolve__list_all();

	var expect_test = new Array;
	expect_test["Removed (B,WC)"] = [ "@c/dir_L/dir_F",
					  "@c/dir_L/dir_G" ];
	expect_test["Removed (C,WC)"] = [ "@b/dir_M/dir_D",
					  "@b/dir_M/dir_E" ];

	expect_test["Existence (A,!B,C,WC)"] = [ "@/dir_L/dir_H",
						 "@/dir_L/dir_I",
						 "@/dir_L/dir_J" ];
	expect_test["Existence (A,B,!C,WC)"] = [ "@/dir_M/dir_A",
						 "@/dir_M/dir_B",
						 "@/dir_M/dir_C" ];

	expect_test["Moved (C!=A,WC==C)"] = [ "@/dir_L/dir_H",
					      "@/dir_L/dir_I",
					      "@/dir_L/dir_J" ];
	expect_test["Moved (B!=A,WC==B)"] = [ "@/dir_M/dir_A",
					      "@/dir_M/dir_B",
					      "@/dir_M/dir_C" ];

	expect_test["Resolved"] = [ "@/dir_M/dir_A",
				    "@/dir_M/dir_B",
				    "@/dir_M/dir_C",
				    "@b/dir_M/dir_D",
				    "@b/dir_M/dir_E",
				    "@c/dir_L/dir_F",
				    "@c/dir_L/dir_G",
				    "@/dir_L/dir_H",
				    "@/dir_L/dir_I",
				    "@/dir_L/dir_J" ];
	expect_test["Choice Resolved (Existence)"] = [ "@/dir_M/dir_A",
						       "@/dir_M/dir_B",
						       "@/dir_M/dir_C",
						       "@b/dir_M/dir_D",
						       "@b/dir_M/dir_E",
						       "@c/dir_L/dir_F",
						       "@c/dir_L/dir_G",
						       "@/dir_L/dir_H",
						       "@/dir_L/dir_I",
						       "@/dir_L/dir_J" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

    }

}
