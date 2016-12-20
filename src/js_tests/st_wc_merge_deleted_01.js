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

function st_wc_merge_deleted_01()
{
    var my_group = "st_wc_merge_deleted_01";	// this variable must match the above group name.

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
	vscript_test_wc__write_file("file_A.txt", content_equal);
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
	vscript_test_wc__addremove();
	vscript_test_wc__write_file("file_A.txt", (content_B1 + content_equal));
	csets.B = vscript_test_wc__commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	sg.wc.checkout( { "repo"   : repoName,
			  "attach" : "master",
			  "rev"    : csets.A
			} );
	this.do_fsobj_mkdir("dir_C");
	vscript_test_wc__addremove();
	vscript_test_wc__write_file("file_A.txt", (content_equal + content_C1));
	csets.C = vscript_test_wc__commit("C");

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Checkout B, and merge C into it (should auto-merge).");

	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.B
				      } );
	var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	// "file_A.txt" was modified differently in B and C
	// and would be an auto-merge

	vscript_test_wc__merge_np( { "rev" : csets.C } );

	var expect_test = new Array;
	expect_test["Modified"]                   = [ "@/file_A.txt" ];
	expect_test["Resolved"]                   = [ "@/file_A.txt" ];
	expect_test["Choice Resolved (Contents)"] = [ "@/file_A.txt" ];
	expect_test["Auto-Merged"]                = [ "@/file_A.txt" ];
	expect_test["Added (Merge)"]              = [ "@/dir_C" ];
        vscript_test_wc__confirm_wii(expect_test);

	// we expect auto-merge to produce this result.
	var mrg = (content_B1 + content_equal + content_C1);
	var f = sg.file.read("file_A.txt");
	testlib.ok( (f == mrg),
		    "Content of file_A.txt." );

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

	vscript_test_wc__remove_file("file_A.txt");

	var expect_test = new Array;
	expect_test["Removed"] = [ "@b/file_A.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__merge_np__expect_error( { "rev" : csets.C }, "This type of merge requires a clean working copy" );
	vscript_test_wc__merge_np( { "rev" : csets.C, "allow_dirty" : true } );

	var path = "@/file_A.txt";

	var expect_test = new Array;
	expect_test["Added (Merge)"]                 = [ "@/dir_C" ];
	expect_test["Modified"]                      = [ path ];
	expect_test["Unresolved"]                    = [ path ];
	expect_test["Choice Unresolved (Existence)"] = [ path ];
        vscript_test_wc__confirm_wii(expect_test);

	// Since we treat pended deletes like real deletes,
	// we SHOULD NOT have auto-merged the result.
	// C should have won.
	var mrg = (content_equal + content_C1);

	// Since there was a conflict and MERGE renamed
	// the file, we have to do a little work to find it.

	var dir = sg.fs.readdir(".");
	for (var k in dir)
	{
	    if (dir[k].isFile)
	    {
		var name = dir[k].name;
		if (name.substr(0,10) == "file_A.txt")
		{
		    var f = sg.file.read( name );
		    testlib.ok( (f == mrg),
				"Content of file_A.txt." );
		}
	    }
	}
    }
}
