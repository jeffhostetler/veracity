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

function st_sparse_06a()
{
    this.my_group = "st_sparse_06a";	// this variable must match the above group name.
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
        this.do_fsobj_create_file("file_4");
        this.do_fsobj_create_file("file_5");
	this.do_fsobj_create_file("sparse_1");
	this.do_fsobj_mkdir(      "sparse_dir_1");
        this.do_fsobj_create_file("sparse_dir_1/sparse_file_1");

	vscript_test_wc__addremove();
	this.do_commit("add initial files");
	var cset_base = sg.wc.parents()[0];

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("WD 2 -- sparse checkout");

	// TODO 2012/11/27 See W5458.  We get different results if there
	// TODO            a trailing slash on the directory in the
	// TODO            sparse-list on the checkout.

	this.my_make_wd();
	sg.wc.checkout( { "repo" : this.repoName,
			  "path" : ".",
			  "sparse" : [ "@/sparse_1",
				       "@/sparse_dir_1" ] } );

        var expect_test = new Array;
	expect_test["Sparse"] = [ "@/sparse_1",
				  "@/sparse_dir_1",
				  "@/sparse_dir_1/sparse_file_1" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Make some dirt....");
	print("");

	this.do_fsobj_append_file("file_3", "Hello!");	// just to have some dirt

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_3" ];
	expect_test["Sparse"] = [ "@/sparse_1",
				  "@/sparse_dir_1",
				  "@/sparse_dir_1/sparse_file_1" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );

	gidPath = sg.wc.get_item_gid_path( { "src" : "@/sparse_1" } );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to remove sparse file...");
	// this should not require --force
	// this should not create a backup file

	var result = sg.exec(vv, "remove", "sparse_1");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "'vv remove sparse_1' should work" );

	var result = sg.exec(vv, "remove", "sparse_dir_1/sparse_file_1");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "'vv remove sparse_dir_1/sparse_file_1' should work" );

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_3" ];
	expect_test["Removed"]  = [ "@b/sparse_1",
				    "@b/sparse_dir_1/sparse_file_1" ];
	expect_test["Sparse"]   = [ "@/sparse_dir_1" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "no_ignores" : true, 
						 "list_sparse" : true } ) );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to revert items individually (not revert-all)...");

	var r = sg.wc.revert_items( { "src" : [ "@/file_3", "@b/sparse_1", "@b/sparse_dir_1/sparse_file_1" ],
				      "no_backups" : true,
				      "verbose" : true } );
	print(sg.to_json__pretty_print(r));

        var expect_test = new Array;
	expect_test["Sparse"]   = [ "@/sparse_1",
				    "@/sparse_dir_1",
				    "@/sparse_dir_1/sparse_file_1" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "no_ignores" : true, 
						 "list_sparse" : true } ) );

	//////////////////////////////////////////////////////////////////
    }

}
