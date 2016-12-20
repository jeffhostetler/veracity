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

function st_wc_merge_resolve_016()
{
    var my_group = "st_wc_merge_resolve_016";	// this variable must match the above group name.

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
	// Create the first WD and initialize the REPO.

	my_make_wd(this);
	sg.vv2.init_new_repo( { "repo" : repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(repoName);

	//////////////////////////////////////////////////////////////////
	// Build a sequence of CSETs using current WD.

	vscript_test_wc__write_file("file_1.txt", content_equal);
	vscript_test_wc__write_file("file_2.txt", content_equal);
	vscript_test_wc__write_file("file_3.txt", content_equal);
	vscript_test_wc__write_file("file_4.txt", content_equal);
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
	vscript_test_wc__write_file("file_1.txt", (content_B1 + content_equal));
	vscript_test_wc__write_file("file_2.txt", (content_B1 + content_equal));
	vscript_test_wc__write_file("file_3.txt", (content_B1 + content_equal));
	vscript_test_wc__write_file("file_4.txt", (content_B1 + content_equal));
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	// re-write the file contents so that they look edited.
	vscript_test_wc__write_file("file_1.txt", (content_C1 + content_equal));
	vscript_test_wc__write_file("file_2.txt", (content_C1 + content_equal));
	vscript_test_wc__write_file("file_3.txt", (content_C1 + content_equal));
	vscript_test_wc__write_file("file_4.txt", (content_C1 + content_equal));
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

	// auto-merge should have been attempted and failed.

	var expect_test = new Array;
	expect_test["Modified (WC!=A,WC!=B,WC!=C)"] = [ "@/file_1.txt",
							"@/file_2.txt",
							"@/file_3.txt",
							"@/file_4.txt" ];
	expect_test["Auto-Merged"] = [ "@/file_1.txt",
				       "@/file_2.txt",
				       "@/file_3.txt",
				       "@/file_4.txt" ];
	expect_test["Unresolved"] = [ "@/file_1.txt",
				      "@/file_2.txt",
				      "@/file_3.txt",
				      "@/file_4.txt" ];
	expect_test["Choice Unresolved (Contents)"] = [ "@/file_1.txt",
							"@/file_2.txt",
							"@/file_3.txt",
							"@/file_4.txt" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 1:");

	vscript_test_wc__resolve__list_all();
	// TODO            vv resolve list --all
	// TODO            Contents: @/file_1.txt
	// TODO            Contents: @/file_2.txt
	// TODO            Contents: @/file_3.txt
	// TODO            Contents: @/file_4.txt

	vscript_test_wc__resolve__accept("baseline", "@/file_1.txt");
	vscript_test_wc__resolve__accept("other",    "@/file_2.txt");
	vscript_test_wc__resolve__accept("ancestor", "@/file_3.txt");

	var expect_test = new Array;
	expect_test["Modified (WC!=A,WC==B,WC!=C)"] = [ "@/file_1.txt" ];
	expect_test["Modified (WC!=A,WC!=B,WC==C)"] = [ "@/file_2.txt" ];
	expect_test["Modified (WC==A,WC!=B,WC!=C)"] = [ "@/file_3.txt" ];
	expect_test["Modified (WC!=A,WC!=B,WC!=C)"] = [ "@/file_4.txt" ];
	expect_test["Auto-Merged (Edited)"] =  [ "@/file_1.txt",
						 "@/file_2.txt",
						 "@/file_3.txt" ];
	expect_test["Auto-Merged"] = [ "@/file_4.txt" ];
	expect_test["Resolved"] =  [ "@/file_1.txt",
				     "@/file_2.txt",
				     "@/file_3.txt" ];
	expect_test["Choice Resolved (Contents)"] =  [ "@/file_1.txt",
						       "@/file_2.txt",
						       "@/file_3.txt" ];
	expect_test["Unresolved"] = [ "@/file_4.txt" ];
	expect_test["Choice Unresolved (Contents)"] = [ "@/file_4.txt" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE 2:");

	vscript_test_wc__resolve__list_all();
	// TODO            vv resolve list --all
	// TODO            Contents: @/file_1.txt
	// TODO            Contents: @/file_2.txt
	// TODO            Contents: @/file_3.txt
	// TODO            Contents: @/file_4.txt

	// overwrite working to say we fixed it and then tell resolve to just use it
	vscript_test_wc__write_file("file_4.txt", (content_B1 + content_C1 + content_equal));

	// TODO 2012/04/12 This requires that _wc__variantFile_to_absolute() 
	// TODO            works with gid-domain paths and that overwrite-cache thing.
	vscript_test_wc__resolve__accept("working",  "@/file_4.txt");

	var expect_test = new Array;
	expect_test["Modified (WC!=A,WC==B,WC!=C)"] = [ "@/file_1.txt" ];
	expect_test["Modified (WC!=A,WC!=B,WC==C)"] = [ "@/file_2.txt" ];
	expect_test["Modified (WC==A,WC!=B,WC!=C)"] = [ "@/file_3.txt" ];
	expect_test["Modified (WC!=A,WC!=B,WC!=C)"] = [ "@/file_4.txt" ];
	expect_test["Auto-Merged (Edited)"] =  [ "@/file_1.txt",
						 "@/file_2.txt",
						 "@/file_3.txt",
						 "@/file_4.txt" ];
	expect_test["Resolved"] =  [ "@/file_1.txt",
				     "@/file_2.txt",
				     "@/file_3.txt",
				     "@/file_4.txt" ];
	expect_test["Choice Resolved (Contents)"] =  [ "@/file_1.txt",
						       "@/file_2.txt",
						       "@/file_3.txt",
						       "@/file_4.txt" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);
	
    }

}
