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

function st_sparse_04()
{
    this.my_group = "st_sparse_04";	// this variable must match the above group name.
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

        this.do_fsobj_create_file("file_3");
	this.do_fsobj_create_file("sparse_1");
	this.do_fsobj_mkdir(      "sparse_dir_1");
        this.do_fsobj_create_file("sparse_dir_1/sparse_file_1");
	this.do_fsobj_mkdir(      "sparse_dir_2");
        this.do_fsobj_create_file("sparse_dir_2/sparse_file_2");
	this.do_fsobj_mkdir(      "dir_3");

	vscript_test_wc__addremove();
	this.do_commit("add initial files");
	var cset_base = sg.wc.parents()[0];

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("WD 2 -- sparse checkout");

	this.my_make_wd();
	sg.wc.checkout( { "repo" : this.repoName,
			  "path" : ".",
			  "sparse" : [ "@/sparse_1",
				       "@/sparse_dir_1",
				       "@/sparse_dir_2" ] } );

        var expect_test = new Array;
	expect_test["Sparse"] = [ "@/sparse_1",
				  "@/sparse_dir_1",
				  "@/sparse_dir_1/sparse_file_1",
				  "@/sparse_dir_2",
				  "@/sparse_dir_2/sparse_file_2" ];
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
				  "@/sparse_dir_1/sparse_file_1",
				  "@/sparse_dir_2",
				  "@/sparse_dir_2/sparse_file_2" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );

	gidPath = sg.wc.get_item_gid_path( { "src" : "@/sparse_1" } );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to move a sparse file...");

	var result = sg.exec(vv, "move", "sparse_1", "dir_3");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "'vv move sparse_1 dir_3' should work" );

	var result = sg.exec(vv, "move", "sparse_dir_1/sparse_file_1", "sparse_dir_2");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "'vv move sparse_dir_1/sparse_file_1 sparse_dir_2' should work" );

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_3" ];
	expect_test["Sparse"] = [ "@/dir_3/sparse_1",
				  "@/sparse_dir_1",
				  "@/sparse_dir_2/sparse_file_1",
				  "@/sparse_dir_2",
				  "@/sparse_dir_2/sparse_file_2" ];
	expect_test["Moved"]  = [ "@/dir_3/sparse_1",
				  "@/sparse_dir_2/sparse_file_1" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "no_ignores" : true, 
						 "list_sparse" : true } ) );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Status dump after move...");
	print(sg.to_json__pretty_print(sg.wc.status({ "list_unchanged" : true,
						      "no_ignores" : true,
						      "list_sparse" : true } )));

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Trying test commit...");

	var o = sg.exec(vv, "commit", "-m", "foo", "--test", "--verbose");
	testlib.ok( (o.exit_status == 0) );
	print("Test commit:");
	print(o.stdout);
	print(o.stderr);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to commit with a dirty sparse file...");

	this.do_commit("commit dirt");

        var expect_test = new Array;
	expect_test["Sparse"] = [ "@/dir_3/sparse_1",
				  "@/sparse_dir_1",
				  "@/sparse_dir_2/sparse_file_1",
				  "@/sparse_dir_2",
				  "@/sparse_dir_2/sparse_file_2" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Status dump after commit...");
	print(sg.to_json__pretty_print(sg.wc.status({ "list_unchanged" : true,
						      "no_ignores" : true,
						      "list_sparse" : true } )));

	//////////////////////////////////////////////////////////////////
    }

}
