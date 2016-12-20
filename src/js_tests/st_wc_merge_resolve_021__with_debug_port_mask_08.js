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

function st_wc_merge_resolve_021()
{
    var my_group = "st_wc_merge_resolve_021";	// this variable must match the above group name.

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
	// stock content for creating files.

	var content_equal = ("This is a test 1.\n"
			     +"This is a test 2.\n"
			     +"This is a test 3.\n"
			     +"This is a test 4.\n"
			     +"This is a test 5.\n"
			     +"This is a test 6.\n"
			     +"This is a test 7.\n"
			     +"This is a test 8.\n");
	var content_B1 = "abcdefghij\n";
	var content_C1 = "0123456789\n";

	//////////////////////////////////////////////////////////////////
	// make sure we don't inherit a funky port-mask

	if (sg.config.debug)
	    vscript_test_wc__set_debug_port_mask__on();

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

	this.do_fsobj_mkdir("dirA1");
	this.do_fsobj_mkdir("dirA2");
	vscript_test_wc__write_file("dirA1/portability.txt", content_equal);
	vscript_test_wc__write_file("dirA2/fileA2.txt",      content_equal);
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__move("dirA1/portability.txt", "dirA2");
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	vscript_test_wc__rename("dirA2/fileA2.txt", "PORTABILITY.TXT");
	csets.C = vscript_test_wc__commit("C");

	//////////////////////////////////////////////////////////////////
	// Do a MERGE.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout B and merge C into it.");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.B
				      } );

	vscript_test_wc__merge_np( { "rev" : csets.C } );

	// Expect portability problem on portability.txt/PORTABILITY.TXT.

	var B_pathname = "@/dirA2/portability.txt~g[0-9a-f]*";
	var C_pathname = "@/dirA2/PORTABILITY.TXT~g[0-9a-f]*";

	var expect_test = new Array;
	expect_test["Renamed"]                       = [ B_pathname, C_pathname ];
	expect_test["Unresolved"]                    = [ B_pathname, C_pathname ];
	expect_test["Choice Unresolved (Location)"]  = [ B_pathname             ];
	expect_test["Choice Unresolved (Name)"]      = [             C_pathname ];
        vscript_test_wc__confirm_wii(expect_test);

	// use MSTATUS

	var o = sg.exec(vv,"mstatus", "--verbose");
	print(o.stdout);

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"]   = [ B_pathname, C_pathname ];
	expect_test["Moved (WC!=A,WC==B,WC!=C)"]     = [ B_pathname             ];
	expect_test["Unresolved"]                    = [ B_pathname, C_pathname ];
	expect_test["Choice Unresolved (Location)"]  = [ B_pathname             ];
	expect_test["Choice Unresolved (Name)"]      = [             C_pathname ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 1:");

	vscript_test_wc__resolve__list_all();

	vscript_test_wc__rename("@b/dirA2/portability.txt",  "B__portability.txt");
	vscript_test_wc__rename("@c/dirA2/PORTABILITY.TXT",  "C__portability.txt");

	vscript_test_wc__print_section_divider("RESOLVE 1: MSTATUS after renames:");
	var o = sg.exec(vv,"mstatus", "--verbose");
	print(o.stdout);

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"]  = [ "@/dirA2/B__portability.txt", "@/dirA2/C__portability.txt" ];
	expect_test["Moved (WC!=A,WC==B,WC!=C)"]    = [ "@/dirA2/B__portability.txt"                               ];
	expect_test["Resolved"]                     = [                               "@/dirA2/C__portability.txt" ];
	expect_test["Choice Resolved (Name)"]       = [                               "@/dirA2/C__portability.txt" ];
	expect_test["Unresolved"]                   = [ "@/dirA2/B__portability.txt"                               ];
	expect_test["Choice Unresolved (Location)"] = [ "@/dirA2/B__portability.txt"                               ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	vscript_test_wc__print_section_divider("RESOLVE 1: Preparing to accept values:");
	vscript_test_wc__resolve__accept("working",    "@b/dirA2/portability.txt", "--overwrite");
	vscript_test_wc__resolve__accept("working",    "@c/dirA2/PORTABILITY.TXT", "--overwrite");

	vscript_test_wc__print_section_divider("RESOLVE 1: MSTATUS after accepts:");
	var o = sg.exec(vv,"mstatus", "--verbose");
	print(o.stdout);

	var expect_test = new Array;
	expect_test["Renamed (WC!=A,WC!=B,WC!=C)"] = [ "@/dirA2/B__portability.txt", "@/dirA2/C__portability.txt" ];
	expect_test["Moved (WC!=A,WC==B,WC!=C)"]   = [ "@/dirA2/B__portability.txt"                               ];
	expect_test["Resolved"]                    = [ "@/dirA2/B__portability.txt", "@/dirA2/C__portability.txt" ];
	expect_test["Choice Resolved (Location)"]  = [ "@/dirA2/B__portability.txt"                               ];
	expect_test["Choice Resolved (Name)"]      = [                               "@/dirA2/C__portability.txt" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all();

    }

}
