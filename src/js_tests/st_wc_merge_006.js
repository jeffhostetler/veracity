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

function st_wc_merge_006()
{
    var my_group = "st_wc_merge_006";	// this variable must match the above group name.

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
	vscript_test_wc__write_file("file_5", (content_C1 + content_equal));
	vscript_test_wc__write_file("file_6", (content_C1 + content_equal));
	vscript_test_wc__write_file("file_7", (content_C1 + content_equal));
	vscript_test_wc__write_file("file_8", (content_C1 + content_equal));
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

	// using plain/traditional STATUS, we expect just the changes relative to B.

	var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_5",
				    "@/file_6",
				    "@/file_7",
				    "@/file_8",
				  ];
        vscript_test_wc__confirm_wii(expect_test);

	// use MSTATUS

	var expect_test = new Array;
	expect_test["Modified (WC!=A,WC==B,WC!=C)"] = [ "@/file_1",
							"@/file_2",
							"@/file_3",
							"@/file_4",
						      ];
	expect_test["Modified (WC!=A,WC!=B,WC==C)"] = [ "@/file_5",
							"@/file_6",
							"@/file_7",
							"@/file_8",
						      ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);
	
    }

}
