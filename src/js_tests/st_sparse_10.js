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

function st_sparse_10()
{
    this.my_group = "st_sparse_10";	// this variable must match the above group name.
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
	vscript_test_wc__print_section_divider("WD 2");

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
	this.do_fsobj_create_file("sparse_2");
	if (bChmod)
	    this.do_fsobj_create_file("sparse_3");
	if (bSymlinks)
	    this.do_make_symlink("file_1", "sparse_link_1");
	vscript_test_wc__addremove();
	var cset_base = vscript_test_wc__commit_np( { "message" : "add initial files" } );

        this.do_fsobj_create_file("file_4");
        this.do_fsobj_create_file("file_5");
	this.do_fsobj_append_file("sparse_2", "more text");
	if (bChmod)
	    this.do_chmod("sparse_3", 0700);
	if (bSymlinks)
	    this.do_make_symlink("file_2", "sparse_link_1");	// retarget it
	vscript_test_wc__addremove();
	var cset_L0 = vscript_test_wc__commit_np( { "message" : "L0 changes" } );

	vscript_test_wc__update_attached(cset_base, "master");
	
        this.do_fsobj_create_file("file_6");
	vscript_test_wc__addremove();
	var cset_L1 = vscript_test_wc__commit_np( { "message" : "L1 changes" } );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("WD 2 -- sparse checkout");

	var sparse_list = [ "@/sparse_1", "@/sparse_2" ];
	if (bChmod)
	    sparse_list.push( "@/sparse_3" );
	if (bSymlinks)
	    sparse_list.push( "@/sparse_link_1" );

	this.my_make_wd();
	sg.wc.checkout( { "repo" : this.repoName,
			  "path" : ".",
			  "rev"  : cset_L0,
			  "sparse" : sparse_list } );

        var expect_test = new Array;
	expect_test["Sparse"] = sparse_list;
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to MERGE with sparse files...");

	var result = sg.exec(vv, "merge", "--rev", cset_L1, "--verbose");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "'vv update' should work" );

        var expect_test = new Array;
	expect_test["Sparse"] = sparse_list;
	expect_test["Added (Merge)"] = [ "@/file_6" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "no_ignores" : true, 
						 "list_sparse" : true } ) );

	//////////////////////////////////////////////////////////////////

    }

}
