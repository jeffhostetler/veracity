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

function st_sparse_10b()
{
    this.my_group = "st_sparse_10b";	// this variable must match the above group name.
    this.no_setup = true;		// do not create an initial REPO and WD.

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    this.unique = this.my_group + "_" + new Date().getTime();

    this.repoName = this.unique;
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

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	var bChmod    = ((sg.platform() == "LINUX") || (sg.platform() == "MAC"));
	var bSymlinks = ((sg.platform() == "LINUX") || (sg.platform() == "MAC"));

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	this.my_make_wd();
	sg.vv2.init_new_repo( { "repo" : this.repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(this.repoName);

        this.do_fsobj_create_file("file_1");
        this.do_fsobj_create_file("file_2");
        this.do_fsobj_create_file("file_3");
	this.do_fsobj_create_file("sparse_1");
	if (bChmod)
	    this.do_fsobj_create_file("sparse_chmod_1");
	if (bSymlinks)
	    this.do_make_symlink("file_1", "sparse_link_1");
	vscript_test_wc__addremove();
	var cset_base = vscript_test_wc__commit_np( { "message" : "add initial files" } );

	//////////////////
	// create
	//       _base
	//      /     \
	//  L0_a       L1_a
	// make same content change in both branches

	vscript_test_wc__update_attached(cset_base, "master");

        this.do_fsobj_create_file("noise_L0_a");
	sg.file.append( "sparse_1", "Hello!\n" );
	if (bChmod)
	    this.do_chmod("sparse_chmod_1", 0700);
	if (bSymlinks)
	    this.do_make_symlink("file_3", "sparse_link_1");	// retarget it
	vscript_test_wc__addremove();
	var cset_L0_a = vscript_test_wc__commit_np( { "message" : "L0_a changes" } );

	vscript_test_wc__update_attached(cset_base, "master");

        this.do_fsobj_create_file("noise_L1_a");
	sg.file.append( "sparse_1", "Hello!\n" );
	if (bChmod)
	    this.do_chmod("sparse_chmod_1", 0700);
	if (bSymlinks)
	    this.do_make_symlink("file_3", "sparse_link_1");	// retarget it
	vscript_test_wc__addremove();
	var cset_L1_a = vscript_test_wc__commit_np( { "message" : "L1_a changes" } );

	//////////////////
	// create
	//       _base
	//      /     \
	//  L0_b       L1_b
	// make different file content changes (which may or may not be auto-mergeable)

	vscript_test_wc__update_attached(cset_base, "master");

	sg.file.append( "sparse_1", "Hello!\n" );
	vscript_test_wc__addremove();
	var cset_L0_b = vscript_test_wc__commit_np( { "message" : "L0_b changes" } );

	vscript_test_wc__update_attached(cset_base, "master");

	sg.file.append( "sparse_1", "There!\n" );
	vscript_test_wc__addremove();
	var cset_L1_b = vscript_test_wc__commit_np( { "message" : "L1_b changes" } );

	//////////////////
	// create
	//       _base
	//      /     \
	//  L0_c       L1_c
	// make different symlink targets (which will cause a conflict)

	if (bSymlinks)
	{
	    vscript_test_wc__update_attached(cset_base, "master");

	    this.do_make_symlink("file_2", "sparse_link_1");	// retarget it
	    vscript_test_wc__addremove();
	    var cset_L0_c = vscript_test_wc__commit_np( { "message" : "L0_c changes" } );

	    vscript_test_wc__update_attached(cset_base, "master");

	    this.do_make_symlink("file_3", "sparse_link_1");	// retarget it
	    vscript_test_wc__addremove();
	    var cset_L1_c = vscript_test_wc__commit_np( { "message" : "L1_c changes" } );
	}

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	var sparse_list = [ "@/sparse_1" ];
	if (bChmod)
	    sparse_list.push( "@/sparse_chmod_1" );
	if (bSymlinks)
	    sparse_list.push( "@/sparse_link_1" );

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	this.my_make_wd();

	vscript_test_wc__print_section_divider("Checkout L0_a");
	sg.wc.checkout( { "repo" : this.repoName,
			  "path" : ".",
			  "rev"  : cset_L0_a,
			  "sparse" : sparse_list } );

        var expect_test = new Array;
	expect_test["Sparse"] = sparse_list;
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );

	print("WD before merge with sparse items...");
	print(sg.to_json__pretty_print(sg.wc.status({"list_sparse":true,
						     "list_unchanged":true,
						     "no_ignores":true})));

	vscript_test_wc__print_section_divider("Merge in L1_a...");
	var result = sg.wc.merge( { "rev" : cset_L1_a,
				    "verbose" : true } );
	print(sg.to_json__pretty_print(result));

	print("WD after merge (L0_a, L1_a) ...");
	print(sg.to_json__pretty_print(sg.wc.status({"list_sparse":true,
						     "list_unchanged":true,
						     "no_ignores":true})));

        var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/noise_L1_a" ];
	expect_test["Sparse"] = sparse_list;
	// nothing should appear changed since both parents made the same change.
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "no_ignores" : true, 
						 "list_sparse" : true } ) );

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	this.my_make_wd();

	vscript_test_wc__print_section_divider("Checkout L0_b");
	sg.wc.checkout( { "repo" : this.repoName,
			  "path" : ".",
			  "rev"  : cset_L0_b,
			  "sparse" : sparse_list } );

        var expect_test = new Array;
	expect_test["Sparse"] = sparse_list;
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );

	print("WD before merge with sparse items...");
	print(sg.to_json__pretty_print(sg.wc.status({"list_sparse":true,
						     "list_unchanged":true,
						     "no_ignores":true})));

	vscript_test_wc__print_section_divider("Merge in L1_b...");

	try
	{
	    var result = sg.wc.merge( { "rev" : cset_L1_b,
					"verbose" : true } );
	    print(sg.to_json__pretty_print(result));
	    testlib.ok( (0), ("Merge should throw since auto-merged file is sparse.") );
	}
	catch (e)
	{
	    print(sg.to_json__pretty_print(e));
	    testlib.ok( (e.message.indexOf("The file '@/sparse_1' needs to be merged, but is sparse.") >= 0) );
	}

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	if (bSymlinks)
	{
	    this.my_make_wd();

	    vscript_test_wc__print_section_divider("Checkout L0_c");
	    sg.wc.checkout( { "repo" : this.repoName,
			      "path" : ".",
			      "rev"  : cset_L0_c,
			      "sparse" : sparse_list } );

            var expect_test = new Array;
	    expect_test["Sparse"] = sparse_list;
            vscript_test_wc__confirm(expect_test,
				     sg.wc.status( { "list_sparse" : true } ) );

	    print("WD before merge with sparse items...");
	    print(sg.to_json__pretty_print(sg.wc.status({"list_sparse":true,
							 "list_unchanged":true,
							 "no_ignores":true})));

	    vscript_test_wc__print_section_divider("Merge in L1_c...");

	    var result = sg.wc.merge( { "rev" : cset_L1_c,
					"verbose" : true } );
	    print(sg.to_json__pretty_print(result));

	    print("WD after merge (L0_c, L1_c) ...");
	    print(sg.to_json__pretty_print(sg.wc.status({"list_sparse":true,
							 "list_unchanged":true,
							 "no_ignores":true})));

            var expect_test = new Array;
	    expect_test["Sparse"] = sparse_list;
	    expect_test["Unresolved"] = [ "@/sparse_link_1" ];
	    expect_test["Choice Unresolved (Contents)"] = [ "@/sparse_link_1" ];
            vscript_test_wc__confirm(expect_test,
				     sg.wc.status( { "no_ignores" : true, 
						     "list_sparse" : true } ) );
	}

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

    }

}
