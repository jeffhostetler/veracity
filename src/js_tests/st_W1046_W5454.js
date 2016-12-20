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
// Explore the new RESERVED status for .sgdrawer.
//////////////////////////////////////////////////////////////////

function st_W1046_W5454()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	print("");
	print("================================================================");
	print("Begin test");
	print("================================================================");
	print("");

        this.workdir_root = sg.fs.getcwd();     // save this for later

        //////////////////////////////////////////////////////////////////
        // fetch the HID of the initial commit.

        this.csets.push( sg.wc.parents()[0] );
        this.names.push( "*Initial Commit*" );

        //////////////////////////////////////////////////////////////////
	
        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_mkdir("dir_2");
        this.do_fsobj_mkdir("dir_3");
        this.do_fsobj_mkdir("dir_4");
        this.do_fsobj_create_file("dir_1/common.txt");
        this.do_fsobj_create_file("dir_2/common.txt");
        vscript_test_wc__addremove();

	this.do_fsobj_create_file("dir_1/another.txt");
	vscript_test_wc__add("dir_1/another.txt");

	this.do_commit("Stuff");

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[1] on .sgdrawer.");
	// None of these operations should do anything.
	// They may or may not report an error (because of
	// we can't complain about .sgdrawer when they do
	// something like use 'vv add *' on Windows).
	// But the status should not change.
	vscript_test_wc__add__ignore_error( ".sgdrawer" );
	vscript_test_wc__add__ignore_error( ".sgdrawer/" );
	vscript_test_wc__add__ignore_error( ".sgdrawer/repo.json" );
	vscript_test_wc__add__ignore_error( ".sgdrawer/foo" );
	vscript_test_wc__move__ignore_error( "dir_1", ".sgdrawer/" );

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[2] on .sgdrawer.");
	// We no longer let STATUS go into reserved directories.
	// So it won't see the new test dir.  And it still won't
	// let us move something into it.
	this.do_fsobj_mkdir(".sgdrawer/test_temp_dir");
	vscript_test_wc__move__ignore_error( "dir_2", ".sgdrawer/test_temp_dir" );

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[3] on .sgdrawer.");
	// get the portability collider involved
	if (sg.platform() == "LINUX")
	{
	    // on Linux, this is a different directory than our real .sgdrawer.
	    // we won't use it, but we still treat it as reserved, so it won't
	    // appear in the status.
	    this.do_fsobj_mkdir(".SGDRAWER");
	    vscript_test_wc__add__ignore_error( ".SGDRAWER" );
	    vscript_test_wc__add__ignore_error( ".SGDRAWER/" );

	    var expect_test = new Array;
	    expect_test["Reserved"] = [ "@/.sgdrawer",
					"@/.SGDRAWER" ];
	    vscript_test_wc__confirm_wii_res(expect_test);

	    sg.fs.rmdir(".SGDRAWER");
	}

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[4] on .sgdrawer.");
	// get the portability collider involved
	if ((sg.platform() == "LINUX") || (sg.platform() == "MAC"))
	{
	    // these names are potentially equivalent, so we treat
	    // them as reserved too.  (final-dot)
	    this.do_fsobj_mkdir(".SGDRAWER.....");
	    vscript_test_wc__add__ignore_error( ".SGDRAWER...../" );
	    vscript_test_wc__move__ignore_error( "dir_3", ".SGDRAWER...../" );

	    this.do_fsobj_mkdir(".sgDrawer...");
	    vscript_test_wc__add__ignore_error( ".sgDrawer.../" );
	    vscript_test_wc__move__ignore_error( "dir_3", ".sgDrawer.../" );

	    var expect_test = new Array;
	    expect_test["Reserved"] = [ "@/.sgdrawer",
					"@/.SGDRAWER.....",
					"@/.sgDrawer..." ];
	    vscript_test_wc__confirm_wii_res(expect_test);

	    sg.fs.rmdir(".SGDRAWER.....");
	    sg.fs.rmdir(".sgDrawer...");
	}

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[5] on .sgdrawer.");
	// get the portability collider involved
	if ((sg.platform() == "WINDOWS") || (sg.platform() == "MAC"))
	{
	    // verify that the code to prevents .sgdrawer from being
	    // (explicitly or implicitly) added isn't fooled by case.

	    vscript_test_wc__add__ignore_error( ".SGDRAWER/" );
	    vscript_test_wc__add__ignore_error( ".SGDRAWER/foo" );

	    vscript_test_wc__move__ignore_error( "dir_3", ".SGDRAWER/" );

	    var expect_test = new Array;
	    expect_test["Reserved"] = [ "@/.sgdrawer" ];
	    vscript_test_wc__confirm_wii_res(expect_test);
	}

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[6] on .sgdrawer.");
	// cd into .sgdrawer and try to add things using relative paths
	// bypassing the various arg checks.
	this.do_fsobj_cd( ".sgdrawer" );
	vscript_test_wc__add__ignore_error( "repo.json" );
	vscript_test_wc__move__ignore_error( "../dir_4", "." );

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	this.do_fsobj_cd( ".." );

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[7] on .sgdrawer.");

	vscript_test_wc__rename__ignore_error( ".sgdrawer", "xxx_sgdrawer" );

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Basic tests[8] on .sgdrawer.");

	this.do_fsobj_mkdir("dirA");
	this.do_fsobj_mkdir("dirA/dirB");
	this.do_fsobj_mkdir("dirA/dirB/dirC");
	this.do_fsobj_mkdir("dirA/dirB/dirC/dirD");
	vscript_test_wc__addremove();

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer" ];
	expect_test["Added"] = [ "@/dirA",
				 "@/dirA/dirB",
				 "@/dirA/dirB/dirC",
				 "@/dirA/dirB/dirC/dirD" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// create a reserved directory deep in the tree.
	this.do_fsobj_mkdir("dirA/dirB/dirC/dirD/.sgcloset");

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	expect_test["Added"] = [ "@/dirA",
				 "@/dirA/dirB",
				 "@/dirA/dirB/dirC",
				 "@/dirA/dirB/dirC/dirD" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// We should be able to remove (undo-add) dirA with
	// the reserved item buried within it because we
	// won't actually delete it -- the added items will
	// be kept and become found items.
	vscript_test_wc__remove("dirA");

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	expect_test["Found"] = [ "@/dirA",
				 "@/dirA/dirB",
				 "@/dirA/dirB/dirC",
				 "@/dirA/dirB/dirC/dirD" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// re-add everything under dirA for additional tests
	vscript_test_wc__addremove();

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	expect_test["Added"] = [ "@/dirA",
				 "@/dirA/dirB",
				 "@/dirA/dirB/dirC",
				 "@/dirA/dirB/dirC/dirD" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// try non-recursive remove of dirD because
	// we have a different code-path.
	vscript_test_wc__remove_np( { "src" : "@/dirA/dirB/dirC/dirD", 
				      "nonrecursive" : true });

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	expect_test["Added"] = [ "@/dirA",
				 "@/dirA/dirB",
				 "@/dirA/dirB/dirC" ];
	expect_test["Found"] = [ "@/dirA/dirB/dirC/dirD" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// re-add dirD and commit everything.
	vscript_test_wc__addremove();

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	expect_test["Added"] = [ "@/dirA",
				 "@/dirA/dirB",
				 "@/dirA/dirB/dirC",
				 "@/dirA/dirB/dirC/dirD" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	this.do_commit("dirA_stuff");

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// try non-recursive delete of dirD (since
	// we no longer qualify for the undo-add stuff).
	//
	// use --keep so that it becomes FOUND and we
	// don't try to actually delete the RESERVED .sgcloset within.
	vscript_test_wc__remove_np( { "src" : "@/dirA/dirB/dirC/dirD", 
				      "nonrecursive" : true,
				      "keep" : true });

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	expect_test["Removed"]  = [ "@b/dirA/dirB/dirC/dirD" ];
	expect_test["Found"]    = [ "@/dirA/dirB/dirC/dirD" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// revert-all to recover dirD (manually delete the FOUND copy
	// and re-create the hack .sgcloset).
	sg.fs.rmdir_recursive( "dirA/dirB/dirC/dirD" );
	vscript_test_wc__revert_all( "recover dirD" );
	this.do_fsobj_mkdir("dirA/dirB/dirC/dirD/.sgcloset");

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// try non-recursive actual delete of committed dirD.
	// this should fail because it contains a reserved dir.
	vscript_test_wc__remove_np__expect_error( { "src" : "@/dirA/dirB/dirC/dirD", 
						    "nonrecursive" : true },
						  "The object cannot be safely removed: Cannot do non-recursive delete");

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	vscript_test_wc__confirm_wii_res(expect_test);

	// try recursive delete of dirA.  This too should fail.
	vscript_test_wc__remove_np__expect_error( { "src" : "@/dirA" },
						  "The object cannot be safely removed: Directory not empty");

	var expect_test = new Array;
	expect_test["Reserved"] = [ "@/.sgdrawer",
				    "@/dirA/dirB/dirC/dirD/.sgcloset" ];
	vscript_test_wc__confirm_wii_res(expect_test);

    }
}
