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

function st_sparse_11b()
{
    this.my_group = "st_sparse_11b";	// this variable must match the above group name.

    this.my_data_dir = sg.fs.getcwd();	// we start in the .../src/js_tests directory.
    this.my_fi = this.my_data_dir + "/data__" + this.my_group + ".fi";

    this.my_fi_test = "../" + this.my_group + "_test.fi";
    this.no_setup = true;		// do not create an initial REPO and WD.

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    this.unique = this.my_group + "_" + new Date().getTime();

    this.repoName = this.unique;
    this.repoNameDup1 = this.repoName + "_dup";
    this.repoNameDup2 = this.repoName + "_dup2";

    this.rootDir = pathCombine(tempDir, this.unique);
    this.wdDirs = [];
    this.nrWD = 0;
    this.my_names = new Array;

    this.my_make_wd = function()
    {
	vscript_test_wc__print_section_divider("WD[" + this.nrWD + "]");
	var new_wd = pathCombine(this.rootDir, "wd_" + this.nrWD);

	print("Creating WD[" + this.nrWD + "]: " + new_wd);
	print("");

	this.do_fsobj_mkdir_recursive( new_wd );
	this.do_fsobj_cd( new_wd );
	this.wdDirs.push( new_wd );
	
	this.nrWD += 1;

	return new_wd;
    }

    this.my_test_it = function( local_repoName, local_tag_L0, local_tag_L1 )
    {
	// Do the various tests that we want to do with in the
	// current WD with the given args.  (And isolate whether
	// we are running on a manually-create or fast-import repo.

	var bWindows = (sg.platform() == "WINDOWS");

	// Symlinks are implicitly made sparse since Windows doesn't support Posix symlinks
	// (and we don't support Windows's Junction Points version of symlinks.
	var sparse_list = [ ];
	if (bWindows)
	    sparse_list.push( "@/sparse_link_1" );

	this.my_make_wd();

	vscript_test_wc__print_section_divider("Running tests using repo='"+local_repoName+"'.");

	sg.wc.checkout( { "repo" : local_repoName,
			  "path" : ".",
			  "tag"  : local_tag_L0 } );	// DO NOT include "sparse" on checkout.

	var s = sg.wc.status({ "list_unchanged":true,
			       "list_sparse":true });
	print(sg.to_json__pretty_print(s));
	var x = sg.wc.get_item_status_flags({"src":"@/sparse_link_1"});
	testlib.ok( (x.isSymlink), "Is link.");

        var expect_test = new Array;
	if (bWindows)
	    expect_test["Sparse"] = sparse_list;
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );

	print("");
	print("Try to MERGE with sparse files...");
	print("");

	var result = sg.wc.merge( { "tag" : local_tag_L1,
				    "verbose" : true } );
	print(sg.to_json__pretty_print(result));

        var expect_test = new Array;
	if (bWindows)
	    expect_test["Sparse"] = sparse_list;
	expect_test["Added (Merge)"] = [ "@/file_6" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "no_ignores" : true, 
						 "list_sparse" : true } ) );

    }

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	// Setup a repo with known content.
	//
	// If on a Unix-based system, create a repo with some symlinks and then fast-export it.
	// If on Windows, populate the repo using fast-import.
	// The idea is that once we get the test working on Unix, we'll checkin the .fi file
	// so that Windows can just use it in subsequent test runs.
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	var bHavePosixSymlinks = ((sg.platform() == "LINUX") || (sg.platform() == "MAC"));
	if (bHavePosixSymlinks)
	{
	    this.my_make_wd();
	    sg.vv2.init_new_repo( { "repo" : this.repoName,
				    "hash" : "SHA1/160",
				    "path" : "."
				  } );
	    whoami_testing(this.repoName);

            this.do_fsobj_create_file("file_1");
            this.do_fsobj_create_file("file_2");
            this.do_fsobj_create_file("file_3");
	    this.do_make_symlink("file_1", "sparse_link_1");
	    vscript_test_wc__addremove();
	    var cset_base = vscript_test_wc__commit_np( { "message" : "add initial files" } );
	    sg.vv2.add_tag( { "rev" : cset_base, "value" : "cset_base" } );

            this.do_fsobj_create_file("file_4");
            this.do_fsobj_create_file("file_5");
	    this.do_make_symlink("file_2", "sparse_link_1");	// retarget it
	    vscript_test_wc__addremove();
	    var cset_L0 = vscript_test_wc__commit_np( { "message" : "L0 changes" } );
	    sg.vv2.add_tag( { "rev" : cset_L0, "value" : "cset_L0" } );

	    vscript_test_wc__update_attached(cset_base, "master");
	
            this.do_fsobj_create_file("file_6");
	    vscript_test_wc__addremove();
	    var cset_L1 = vscript_test_wc__commit_np( { "message" : "L1 changes" } );
	    sg.vv2.add_tag( { "rev" : cset_L1, "value" : "cset_L1" } );

	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////
	    // WARNING: if you change anything in the above setup, run the test
	    // WARNING: on Linux/Mac and use the .fi file we generate below to
	    // WARNING: update the data__*.fi file we reference at the top and
	    // WARNING: check it in.
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////

	    // Fast-export what we just created.
	    vscript_test_wc__print_section_divider("Trying to fast-export repo '" + this.repoName + "'...");
	    var r = sg.exec(vv, "fast-export", this.repoName, this.my_fi_test);
	    testlib.ok( (r.exit_status == 0), "Fast-Export to my_fi_test.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));

	    // Fast-import the .fi we just created.
	    vscript_test_wc__print_section_divider("Trying to fast-import repo into '" + this.repoNameDup1 + "'...");
	    var r = sg.exec(vv, "fast-import", this.my_fi_test, this.repoNameDup1);
	    testlib.ok( (r.exit_status == 0), "Fast-Import from my_fi_test.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));

	    // Fast-import the .fi we have under version control.
	    vscript_test_wc__print_section_divider("Trying to fast-import repo into '" + this.repoNameDup2 + "'...");
	    var r = sg.exec(vv, "fast-import", this.my_fi, this.repoNameDup2);
	    testlib.ok( (r.exit_status == 0), "Fast-Import from my_fi.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));

	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////

	    this.my_test_it( this.repoName,     "cset_L0", "cset_L1" );
	    this.my_test_it( this.repoNameDup1, "cset_L0", "cset_L1" );
	    this.my_test_it( this.repoNameDup2, "cset_L0", "cset_L1" );
	}
	else
	{
	    // Since Windows cannot create symlinks, we skip actually building
	    // a new repo and testing it.  Just fast-import the .fi we have
	    // under version control.
	    vscript_test_wc__print_section_divider("Trying to fast-import repo into '" + this.repoNameDup2 + "'...");
	    var r = sg.exec(vv, "fast-import", this.my_fi, this.repoNameDup2);
	    testlib.ok( (r.exit_status == 0), "Fast-Import from my_fi.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));
	    
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////

	    this.my_test_it( this.repoNameDup2, "cset_L0", "cset_L1" );
	}

	//////////////////////////////////////////////////////////////////

    }

}
