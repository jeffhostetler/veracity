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
// See W4447.
//////////////////////////////////////////////////////////////////

function st_update_dirty__W4447() 
{
    load("update_helpers.js");		// load the helper functions
    initialize_update_helpers(this);	// initialize helper functions

    this.SG_ERR_UPDATE_CONFLICT = "Conflicts or changes in the working copy prevented UPDATE";	// text of error 192.

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	var my_test_dir  = "test_1" + "_" + new Date().getTime();
	var my_test_root = pathCombine(tempDir, my_test_dir);

	if (!sg.fs.exists(my_test_root))
	    this.do_fsobj_mkdir_recursive(my_test_root);
	this.do_fsobj_cd(my_test_root);
	sg.vv2.init_new_repo( { "repo" : my_test_dir, "hash" : "SHA1/160" } );
	whoami_testing( my_test_dir );

	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	this.do_commit("Added AAA");

	this.do_fsobj_mkdir("BBB");
	vscript_test_wc__addremove();
	this.do_commit("Added BBB");

	this.do_fsobj_mkdir("CCC");
	vscript_test_wc__addremove();
	this.do_commit("Added CCC");

	//////////////////////////////////////////////////////////////////
	// Create a found file in a found directory and try to do
	// a DIRTY UPDATE and drag them along.
	//////////////////////////////////////////////////////////////////

	this.do_fsobj_mkdir("test");
	this.do_fsobj_create_file("test/ignore.me");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/test", "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	this.do_update_when_dirty_by_name("Added BBB");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/test", "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// Create a .vvignores file and mark the file as ignorable.
	// Then do another DIRTY UPDATE and see if we can still drag it along.
	//////////////////////////////////////////////////////////////////

	sg.file.write(".vvignores", "*.me");
	vscript_test_wc__add(".vvignores");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/.vvignores" ];
	expect_test["Found"] = [ "@/test", "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	var expect_test = new Array;
	expect_test["Added"]     = [ "@/.vvignores" ];
	expect_test["Found"]     = [ "@/test" ];
	expect_test["Ignored"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm(expect_test);

	this.do_update_when_dirty_by_name("Added CCC");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/.vvignores" ];
	expect_test["Found"] = [ "@/test", "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	var expect_test = new Array;
	expect_test["Added"]     = [ "@/.vvignores" ];
	expect_test["Found"]     = [ "@/test" ];
	expect_test["Ignored"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm(expect_test);
    }

    //////////////////////////////////////////////////////////////////

    this.test_2 = function()
    {
	var my_test_dir  = "test_2" + "_" + new Date().getTime();
	var my_test_root = pathCombine(tempDir, my_test_dir);

	if (!sg.fs.exists(my_test_root))
	    this.do_fsobj_mkdir_recursive(my_test_root);
	this.do_fsobj_cd(my_test_root);
	sg.vv2.init_new_repo( { "repo" : my_test_dir, "hash" : "SHA1/160" } );
	whoami_testing( my_test_dir );

	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	this.do_commit("Added AAA");

	this.do_fsobj_mkdir("BBB");
	vscript_test_wc__addremove();
	this.do_commit("Added BBB");

	this.do_fsobj_mkdir("CCC");
	vscript_test_wc__addremove();
	this.do_commit("Added CCC");

	//////////////////////////////////////////////////////////////////
	// Create a found file in an added directory and try to do
	// a DIRTY UPDATE and drag them along.
	//////////////////////////////////////////////////////////////////

	this.do_fsobj_mkdir("test");
	vscript_test_wc__add("test");
	this.do_fsobj_create_file("test/ignore.me");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/test" ];
	expect_test["Found"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	this.do_update_when_dirty_by_name("Added BBB");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/test" ];
	expect_test["Found"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// Create a .vvignores file and mark the file as ignorable.
	// Then do another DIRTY UPDATE and see if we can still drag it along.
	//////////////////////////////////////////////////////////////////

	sg.file.write(".vvignores", "*.me");
	vscript_test_wc__add(".vvignores");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/.vvignores", "@/test" ];
	expect_test["Found"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	var expect_test = new Array;
	expect_test["Added"]     = [ "@/.vvignores", "@/test" ];
	expect_test["Ignored"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm(expect_test);

	this.do_update_when_dirty_by_name("Added CCC");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/.vvignores", "@/test" ];
	expect_test["Found"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	var expect_test = new Array;
	expect_test["Added"]     = [ "@/.vvignores", "@/test" ];
	expect_test["Ignored"] = [ "@/test/ignore.me" ];
        vscript_test_wc__confirm(expect_test);
    }

    //////////////////////////////////////////////////////////////////

    this.test_3 = function()
    {
	var my_test_dir  = "test_3" + "_" + new Date().getTime();
	var my_test_root = pathCombine(tempDir, my_test_dir);

	if (!sg.fs.exists(my_test_root))
	    this.do_fsobj_mkdir_recursive(my_test_root);
	this.do_fsobj_cd(my_test_root);
	sg.vv2.init_new_repo( { "repo" : my_test_dir, "hash" : "SHA1/160" } );
	whoami_testing( my_test_dir );

	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	this.do_commit("Added AAA");

	this.do_fsobj_mkdir("BBB");
	vscript_test_wc__addremove();
	this.do_commit("Added BBB");

	this.do_fsobj_mkdir("CCC");
	vscript_test_wc__addremove();
	this.do_commit("Added CCC");

	//////////////////////////////////////////////////////////////////
	// Create a found file in an added directory (relative to the target
	// cset) and try to do a DIRTY UPDATE and drag it along.
	//////////////////////////////////////////////////////////////////

	this.do_fsobj_create_file("CCC/ignore.me");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/CCC/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

        globalThis = this;
	var fnstr = formatFunctionString("globalThis.do_update_when_dirty_by_name", [ "Added BBB" ] );
	testlib.verifyFailure( fnstr,
			       "Dirty update to BBB should fail because delete of CCC would orphan file.",
			       "The item '@/CCC/ignore.me' would become orphaned.");

	var expect_test = new Array;
	expect_test["Found"] = [ "@/CCC/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// Create a .vvignores file and mark the file as ignorable.
	// Then do another DIRTY UPDATE and see if we can still drag it along.
	//////////////////////////////////////////////////////////////////

	sg.file.write(".vvignores", "*.me");
	vscript_test_wc__add(".vvignores");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/.vvignores" ];
	expect_test["Found"] = [ "@/CCC/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	var expect_test = new Array;
	expect_test["Added"]     = [ "@/.vvignores" ];
	expect_test["Ignored"] = [ "@/CCC/ignore.me" ];
        vscript_test_wc__confirm(expect_test);

        globalThis = this;
	var fnstr = formatFunctionString("globalThis.do_update_when_dirty_by_name", [ "Added BBB" ] );
	testlib.verifyFailure( fnstr,
			       "Dirty update to BBB should fail because delete of CCC would orphan file.",
			       "The item '@/CCC/ignore.me' would become orphaned.");

	var expect_test = new Array;
	expect_test["Added"] = [ "@/.vvignores" ];
	expect_test["Found"] = [ "@/CCC/ignore.me" ];
        vscript_test_wc__confirm_wii(expect_test);

	var expect_test = new Array;
	expect_test["Added"]     = [ "@/.vvignores" ];
	expect_test["Ignored"] = [ "@/CCC/ignore.me" ];
        vscript_test_wc__confirm(expect_test);
    }
}
