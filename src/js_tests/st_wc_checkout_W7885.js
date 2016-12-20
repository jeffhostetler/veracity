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

function st_wc_checkout_W7885()
{
    var my_group = "st_wc_checkout_W7885";	// this variable must match the above group name.

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////

    if (sg.config.debug)
    this.test_it = function()
    {
	var unique = my_group + "_" + new Date().getTime();

	var repoName = unique;
	var rootDir = pathCombine(tempDir, unique);
	var wdDirs = [];
	var csets = {};
	var nrWD = 0;

	var my_make_wd_path = function(obj)
	{
	    var new_wd = pathCombine(rootDir, "wd_" + nrWD);
	    wdDirs.push( new_wd );

	    vscript_test_wc__print_section_divider("Creating WD[" + nrWD + "]: " + new_wd);
	    nrWD += 1;

	    return new_wd;
	}

	var my_make_wd = function(obj)
	{
	    new_wd = my_make_wd_path();

	    obj.do_fsobj_mkdir_recursive( new_wd );
	    obj.do_fsobj_cd( new_wd );

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

	var E1 = "gf2e335b610fa4318a6f5e139230f955c064847bed0fc11e19b53002500da2b78.test";
	var E2 = "gb54c5ca2cfe349eaa3ce599100d1c4d737052960d1e411e1a51e002500da2b78.test";

	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	my_make_wd(this);
	sg.vv2.init_new_repo( { "repo" : repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(repoName);

	vscript_test_wc__write_file("file_1.txt", content_equal);
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	vscript_test_wc__write_file( E1, content_equal );
	vscript_test_wc__addremove();
	csets.E1 = vscript_test_wc__commit("E1");

	//////////////////////////////////////////////////////////////////
	// Try a series of checkouts.

	// This one is the control.  It should succeed.
	my_make_wd(this);
	vscript_test_wc__checkout_np( { "repo"   : repoName,
					"attach" : "master",
					"rev"    : csets.A } );

	//////////////////////////////////////////////////////////////////
	// cset E1 has the file E1.  When debug is enabled, the
	// checkout code will fail during the populate-wd phase.
	// this could simulates a hard-error like a disk-full error.
	// verify that CHECKOUT cleaned up its mess -- it should
	// have deleted the Drawer and wc.db *AND* anything that it
	// created in the intially empty WD.  BUT it should not
	// delete the WD since it didn't create it.
	//
	// create a new empty WD and CD into it.
	// and effectively "vv checkout <repo> -b master -r <rev> ."

	var path_wd = my_make_wd(this);
	vscript_test_wc__checkout_np__expect_error( { "repo"   : repoName,
						      "attach" : "master",
						      "rev"    : csets.E1 },
						    "Checkout: " + E1 );
	testlib.notOnDisk(pathCombine(path_wd, ".sgdrawer/wc.db"));
	testlib.notOnDisk(pathCombine(path_wd, ".sgdrawer"));
	testlib.notOnDisk(pathCombine(path_wd, "file_1.txt"));
	testlib.existsOnDisk(path_wd);

	//////////////////////////////////////////////////////////////////
	// do the same test, but let CHECKOUT create the WD.
	// effectively "vv checkout <repo> -b master -r <rev> <path>"
	// so it should delete everything -- including the WD top.

	var path_wd = my_make_wd_path(this);
	this.do_fsobj_cd( rootDir );

	vscript_test_wc__checkout_np__expect_error( { "repo"   : repoName,
						      "attach" : "master",
						      "rev"    : csets.E1,
						      "path"   : path_wd },
						    "Checkout: " + E1 );
	testlib.notOnDisk(pathCombine(path_wd, ".sgdrawer/wc.db"));
	testlib.notOnDisk(pathCombine(path_wd, ".sgdrawer"));
	testlib.notOnDisk(pathCombine(path_wd, "file_1.txt"));
	testlib.notOnDisk(path_wd);

	//////////////////////////////////////////////////////////////////
	// See if an INIT on a non-empty directory will clean up the
	// repo and drawer but not delete existing files.
	
	var gid_repo_name = sg.gid();

	var path_wd = my_make_wd(this);
	vscript_test_wc__write_file("file_1.txt", content_equal);
	vscript_test_wc__write_file( E2, content_equal );
	vscript_test_wc__init_np__expect_error( { "repo" : gid_repo_name,
						  "hash" : "SHA1/160",
						  "path" : "." },
						"Init: " + E2 );
	testlib.notOnDisk(pathCombine(path_wd, ".sgdrawer/wc.db"));
	testlib.notOnDisk(pathCombine(path_wd, ".sgdrawer"));
	testlib.existsOnDisk(pathCombine(path_wd, "file_1.txt"));
	testlib.existsOnDisk(pathCombine(path_wd, E2));
	testlib.existsOnDisk(path_wd);
	var r = sg.exec(vv, "repos", "list");
	testlib.ok( (r.stdout.indexOf(E2) == -1), "Repo should have been deleted if created." );

    }

}
