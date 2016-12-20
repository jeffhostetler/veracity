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

function st_wc_W7420_W7033()
{
    var my_group = "st_wc_W7420_W7033";	// this variable must match the above group name.

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

	var my__resolve__is_resolved = function( file )
	{
	    indentLine("=> resolve list --all --verbose '" + file + "':");

	    var r = sg.exec(vv, "resolve", "list", "--all", "--verbose", file);

	    print(r.stdout);
	    print(r.stderr);

	    var regex = new RegExp("Status:[ \t]*Resolved");
	    var bIsResolved = regex.test( r.stdout );
	    
	    testlib.ok( (bIsResolved), "Expect '" + file + "' to be resolved." );
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

	vscript_test_wc__write_file("file_0", content_equal);
	vscript_test_wc__write_file("file_1", content_equal);
	vscript_test_wc__write_file("file_2", content_equal);
	vscript_test_wc__write_file("file_3", content_equal);
	vscript_test_wc__write_file("file_4", content_equal);
	vscript_test_wc__write_file("file_5", content_equal);
	vscript_test_wc__write_file("file_6", content_equal);
	vscript_test_wc__write_file("file_7", content_equal);
	vscript_test_wc__write_file("file_8", content_equal);
	vscript_test_wc__write_file("file_9", content_equal);
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	// re-write the file contents so that they look edited.
	vscript_test_wc__write_file("file_1", (content_B1 + content_equal));
	vscript_test_wc__write_file("file_2", (content_B1 + content_equal));
	vscript_test_wc__write_file("file_3", (content_B1 + content_equal));
	vscript_test_wc__write_file("file_4", (content_B1 + content_equal));
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	// re-write the file contents so that they look edited.
	vscript_test_wc__write_file("file_1", (content_equal + content_C1));
	vscript_test_wc__write_file("file_2", (content_equal + content_C1));
	vscript_test_wc__write_file("file_3", (content_equal + content_C1));
	vscript_test_wc__write_file("file_4", (content_equal + content_C1));
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

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE LIST ALL (1)");
	vscript_test_wc__resolve__list_all();

	// using plain/traditional STATUS, we expect just the changes relative to B.
	//
	// Since B put unique content at the front and C put it at the end
	// of a large common area, we expect auto-merge to succeed.

	var expect_test = new Array;
	expect_test["Modified"]                   = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Auto-Merged"]                = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Resolved"]                   = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Choice Resolved (Contents)"] = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
        vscript_test_wc__confirm_wii(expect_test);

	// use MSTATUS
	
	var expect_test = new Array;
	expect_test["Modified (WC!=A,WC!=B,WC!=C)"] = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Auto-Merged"]                  = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Resolved"]                     = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Choice Resolved (Contents)"]   = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("IS_RESOLVED (1)");
	print("================================================================");

	my__resolve__is_resolved( "file_1" );
	my__resolve__is_resolved( "file_2" );
	my__resolve__is_resolved( "file_3" );
	my__resolve__is_resolved( "file_4" );

	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("Creating dirt....");
	print("================================================================");

	// dirty the file post-merge and check with MSTATUS.
	// (again we just re-write them with the new content.)

	// edit the file to look like it did in L0 (aka take-left).
	vscript_test_wc__write_file("file_1", (content_B1 + content_equal));

	// edit the file to look like it did in L1 (aka take-right).
	vscript_test_wc__write_file("file_2", (content_equal + content_C1));

	// edit the file to look like it did in A (aka take-neither).
	vscript_test_wc__write_file("file_3", (content_equal));

	// edit the file after the auto-merge.
	vscript_test_wc__write_file("file_4", (content_B1 + content_equal + content_C1 + content_W1));

	var expect_test = new Array;
	expect_test["Modified (WC!=A,WC==B,WC!=C)"] = [ "@/file_1" ];
	expect_test["Modified (WC!=A,WC!=B,WC==C)"] = [ "@/file_2" ];
	expect_test["Modified (WC==A,WC!=B,WC!=C)"] = [ "@/file_3" ];
	expect_test["Modified (WC!=A,WC!=B,WC!=C)"] = [ "@/file_4" ];
	expect_test["Auto-Merged (Edited)"]         = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Resolved"]                     = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	expect_test["Choice Resolved (Contents)"]   = [ "@/file_1", "@/file_2", "@/file_3", "@/file_4" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE LIST ALL (2)");
	vscript_test_wc__resolve__list_all();

	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("IS_RESOLVED (2)");
	print("================================================================");

	my__resolve__is_resolved( "file_1" );
	my__resolve__is_resolved( "file_2" );
	my__resolve__is_resolved( "file_3" );
	my__resolve__is_resolved( "file_4" );

    }

}
