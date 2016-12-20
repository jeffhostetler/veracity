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
// Basic MERGE/RESOLVE tests that DO NOT require UPDATE nor REVERT.
//////////////////////////////////////////////////////////////////

function st_wc_merge_resolve_001rr()
{
    var my_group = "st_wc_merge_resolve_001rr";	// this variable must match the above group name.

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
	vscript_test_wc__addremove();
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__rename("dir_A", "C__dir_A");
	vscript_test_wc__rename("dir_B", "C__dir_B");
	vscript_test_wc__rename("dir_C", "C__dir_C");
	vscript_test_wc__rename("dir_D", "C__dir_D");
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
	vscript_test_wc__merge_np( { "rev" : csets.C } );

	// DIVERGENT-RENAME conflict
	// see what a regular STATUS says.

	var expect_test = new Array;
	expect_test["Renamed"]       = [ "@/B__dir_A~g[0-9a-f]*",
					 "@/B__dir_B~g[0-9a-f]*",
					 "@/B__dir_C~g[0-9a-f]*",
					 "@/B__dir_D~g[0-9a-f]*" ];
	expect_test["Unresolved"]    = [ "@/B__dir_A~g[0-9a-f]*",
					 "@/B__dir_B~g[0-9a-f]*",
					 "@/B__dir_C~g[0-9a-f]*",
					 "@/B__dir_D~g[0-9a-f]*" ];
	expect_test["Choice Unresolved (Name)"]    = [ "@/B__dir_A~g[0-9a-f]*",
						       "@/B__dir_B~g[0-9a-f]*",
						       "@/B__dir_C~g[0-9a-f]*",
						       "@/B__dir_D~g[0-9a-f]*" ];
        vscript_test_wc__confirm_wii(expect_test);

	// see what the new MSTATUS says.

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"]    = [ "@/B__dir_A~g[0-9a-f]*",
							  "@/B__dir_B~g[0-9a-f]*",
							  "@/B__dir_C~g[0-9a-f]*",
							  "@/B__dir_D~g[0-9a-f]*" ];
	expect_test["Unresolved"]    = [ "@/B__dir_A~g[0-9a-f]*",
					 "@/B__dir_B~g[0-9a-f]*",
					 "@/B__dir_C~g[0-9a-f]*",
					 "@/B__dir_D~g[0-9a-f]*" ];
	expect_test["Choice Unresolved (Name)"]    = [ "@/B__dir_A~g[0-9a-f]*",
						       "@/B__dir_B~g[0-9a-f]*",
						       "@/B__dir_C~g[0-9a-f]*",
						       "@/B__dir_D~g[0-9a-f]*" ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 1:");

	vscript_test_wc__resolve__list_all();
	// TODO            vv resolve list --all
	// TODO            Name: @/B__dir_A~g[0-9a-f]*
	// TODO            Name: @/B__dir_B~g[0-9a-f]*
	// TODO            Name: @/B__dir_C~g[0-9a-f]*
	// TODO            Name: @/B__dir_D~g[0-9a-f]*

	// Use @b syntax to avoid needing to use wildcards in command.
	vscript_test_wc__resolve__accept("baseline", "@b/B__dir_A");	// ==> B__dir_A
	vscript_test_wc__resolve__accept("other",    "@b/B__dir_B");	// ==> C__dir_B
	vscript_test_wc__resolve__accept("ancestor", "@b/B__dir_C");	// ==> dir_C

	// choose final name manually and tell resolve to use the current value
	vscript_test_wc__rename("@b/B__dir_D", "W__dir_D");
	// With W5226, the RENAME will implicilty resolve the location choice, so
	// we don't need to explicitly do an accept; but if we do, we now need
	// to use "--overwrite".
	vscript_test_wc__resolve__accept("working",  "@/W__dir_D", "--overwrite");

	vscript_test_wc__resolve__list_all();
	// vv resolve list --all
	// Name: @/B__dir_A
	// Name: @/C__dir_B
	// Name: @/dir_C
	// Name: @/W__dir_D

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC==B,WC!=C)"]  = [ "@/B__dir_A" ];
	expect_test["Renamed (WC!=A,WC!=B,WC==C)"]  = [ "@/C__dir_B" ];
	expect_test["Renamed (WC==A,WC!=B,WC!=C)"]  = [ "@/dir_C" ];
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"]  = [ "@/W__dir_D" ];
	expect_test["Resolved"]      = [ "@/B__dir_A",
					 "@/C__dir_B",
					 "@/dir_C",
					 "@/W__dir_D" ];
	expect_test["Choice Resolved (Name)"]      = [ "@/B__dir_A",
						       "@/C__dir_B",
						       "@/dir_C",
						       "@/W__dir_D" ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

    }

}
