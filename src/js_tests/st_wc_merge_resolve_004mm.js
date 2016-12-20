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

function st_wc_merge_resolve_003mm()
{
    var my_group = "st_wc_merge_resolve_003mm";	// this variable must match the above group name.

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
	this.do_fsobj_mkdir("QQQ0");
	this.do_fsobj_mkdir("QQQ0/QQQ1");
	this.do_fsobj_mkdir("QQQ0/QQQ1/QQQ2");
	this.do_fsobj_mkdir("QQQ0/QQQ1/QQQ2/QQQ3");
	this.do_fsobj_mkdir("QQQ0/QQQ1/QQQ2/QQQ3/B__");
	this.do_fsobj_mkdir("QQQ0/QQQ1/QQQ2/QQQ3/C__");
	this.do_fsobj_mkdir("QQQ0/QQQ1/QQQ2/QQQ3/W__");
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__move("dir_A", "QQQ0/QQQ1/QQQ2/QQQ3/B__");
	vscript_test_wc__remove("@/QQQ0/QQQ1/QQQ2/QQQ3/C__");
	vscript_test_wc__addremove();
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__move("dir_A", "QQQ0/QQQ1/QQQ2/QQQ3/C__");
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

	// DIVERGENT-MOVE conflict
	// see what a regular STATUS says.

	var expect_test = new Array;
	expect_test["Unresolved"]    = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Choice Unresolved (Location)"]    = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
        vscript_test_wc__confirm_wii(expect_test);

	// see what the new MSTATUS says.

	var expect_test = new Array;
	expect_test["Moved (WC!=A,WC==B,WC!=C)"]    = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Unresolved"]    = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Choice Unresolved (Location)"]    = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Removed (B,WC)"] = [ "@c/QQQ0/QQQ1/QQQ2/QQQ3/C__" ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 1:");

	vscript_test_wc__resolve__list_all();
	// vv resolve list --all
	// Location: @/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A

	// Use @b syntax to avoid needing to use wildcards in command.
	vscript_test_wc__resolve__accept("baseline", "@b/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A");

	vscript_test_wc__resolve__list_all();
	// vv resolve list --all
	// Location:  @/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A/
	//                 # accepted baseline

	var expect_test = new Array;
	expect_test["Moved (WC!=A,WC==B,WC!=C)"]  = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Resolved"]      = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Choice Resolved (Location)"]      = [ "@/QQQ0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Removed (B,WC)"] = [ "@c/QQQ0/QQQ1/QQQ2/QQQ3/C__" ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 2:");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 3:");

	vscript_test_wc__rename("@/QQQ0", "RRR0");

	vscript_test_wc__resolve__list_all();
	// vv resolve list --all
	// Location:  @/RRR0/QQQ1/QQQ2/QQQ3/B__/dir_A/
	//                 # accepted baseline

	vscript_test_wc__resolve__list_all_verbose();
	// vv resolve list --all
	// Item:        @/RRR0/QQQ1/QQQ2/QQQ3/B__/dir_A/
	// Conflict:    Location
	//              You must choose a location/parent for the item.
	// Cause:       Move/Move
	//              Item moved to different places in different branches.
	// Status:      Resolved (accepted baseline)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: @/
	//    baseline: @/RRR0/QQQ1/QQQ2/QQQ3/B__/      -- note RRR0 in path since
	//       other: @g123456....                    -- need W7521
	//     working: @/RRR0/QQQ1/QQQ2/QQQ3/B__/

	var expect_test = new Array;
	expect_test["Removed (B,WC)"] = [ "@c/QQQ0/QQQ1/QQQ2/QQQ3/C__" ];
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"] = [ "@/RRR0" ];
	expect_test["Moved (WC!=A,WC==B,WC!=C)"]  = [ "@/RRR0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Resolved"]      = [ "@/RRR0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
	expect_test["Choice Resolved (Location)"]      = [ "@/RRR0/QQQ1/QQQ2/QQQ3/B__/dir_A" ];
        vscript_test_wc__mstatus_confirm_wii(expect_test);

    }

}
