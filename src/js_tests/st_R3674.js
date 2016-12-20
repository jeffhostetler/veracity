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

function st_R3674()
{
    var my_group = "st_R3674";	// this variable must match the above group name.

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
	var gids = {};
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
	var content_W1 = "qwertyuiop\n";

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
	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_X");
	vscript_test_wc__write_file("dir_X/file_B.txt", (content_B1));
	vscript_test_wc__addremove();
	csets.B = vscript_test_wc__commit("B");
	gids.B_dir_X  = sg.wc.get_item_gid_path({"src":"@/dir_X"});
	gids.B_file_B = sg.wc.get_item_gid_path({"src":"@/dir_X/file_B.txt"});
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_X");
	vscript_test_wc__write_file("dir_X/file_C.txt", (content_C1));
	vscript_test_wc__addremove();
	csets.C = vscript_test_wc__commit("C");
	gids.C_dir_X  = sg.wc.get_item_gid_path({"src":"@/dir_X"});
	gids.C_file_C = sg.wc.get_item_gid_path({"src":"@/dir_X/file_C.txt"});

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////

	my_make_wd(this);
	vscript_test_wc__print_section_divider("Checkout B, and merge C into it.");
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.B
				      } );
	var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	print("Merge C into B:");
	vscript_test_wc__merge_np( { "rev" : csets.C } );

	var o = sg.exec(vv, "status", "--verbose");
	print(o.stdout);

	var expect_test = new Array;
	expect_test["Added (Merge)"]                 = [ gids.C_dir_X, gids.C_file_C ];
	expect_test["Renamed"]                       = [ gids.B_dir_X ];
	expect_test["Unresolved"]                    = [ gids.B_dir_X, gids.C_dir_X ];
	expect_test["Choice Unresolved (Existence)"] = [ gids.B_dir_X, gids.C_dir_X ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("Pick one of the dir_X's as the one to keep.");
	vscript_test_wc__resolve__accept("baseline", gids.B_dir_X);
	
	var expect_test = new Array;
	expect_test["Added (Merge)"]                 = [ gids.C_dir_X, gids.C_file_C ];
	expect_test["Resolved"]                      = [ gids.B_dir_X ];
	expect_test["Choice Resolved (Existence)"]   = [ gids.B_dir_X ];
	expect_test["Unresolved"]                    = [ gids.C_dir_X ];
	expect_test["Choice Unresolved (Existence)"] = [ gids.C_dir_X ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("Move file_C into kept dir_X.");
	vscript_test_wc__move( gids.C_file_C, gids.B_dir_X );

	//////////////////////////////////////////////////////////////////
	print("");
	print("Remove the other dir_X (should implicitly resolve it).");
	vscript_test_wc__remove_dir( gids.C_dir_X );

	//////////////////////////////////////////////////////////////////
	
	var o = sg.exec(vv, "status", "--verbose");
	print(o.stdout);

	var expect_test = new Array;
	expect_test["Added (Merge)"]                 = [ gids.C_dir_X, gids.C_file_C ];
	expect_test["Resolved"]                      = [ gids.B_dir_X, gids.C_dir_X ];
	expect_test["Choice Resolved (Existence)"]   = [ gids.B_dir_X, gids.C_dir_X ];
	expect_test["Removed"]                       = [ gids.C_dir_X ];
	// We wont' get a "moved" status on the file because it wasn't present
	// in the baseline and so it doesn't have a reference location.
	// expect_test["Moved"]                         = [ gids.C_file_C ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try revert to test R3674.");

	vscript_test_wc__revert_all__np( {"no_backups":true, "verbose":true} );

	var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

    }
}
