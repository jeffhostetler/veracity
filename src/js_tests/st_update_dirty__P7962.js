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

function st_update_dirty__P7962() 
{
    load("update_helpers.js");		// load the helper functions
    initialize_update_helpers(this);	// initialize helper functions

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	var my_test_dir  = "test_1" + "_" + new Date().getTime();
	var my_test_root = pathCombine(tempDir, my_test_dir);
	var cset = new Array();

	if (!sg.fs.exists(my_test_root))
	    this.do_fsobj_mkdir_recursive(my_test_root);
	this.do_fsobj_cd(my_test_root);
	sg.vv2.init_new_repo( { "repo" : my_test_dir, "hash" : "SHA1/160" } );
	whoami_testing( my_test_dir );

	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	cset["AAA"] = vscript_test_wc__commit("Added AAA");

	this.do_fsobj_mkdir("BBB");
	vscript_test_wc__addremove();
	cset["BBB"] = vscript_test_wc__commit("Added BBB");

	this.do_fsobj_mkdir("CCC");
	this.do_fsobj_create_file("test.file");
	vscript_test_wc__addremove();
	cset["CCC"] = vscript_test_wc__commit("Added CCC");

	//////////////////////////////////////////////////////////////////

	vscript_test_wc__move("test.file", "CCC");

	var expect_test = new Array;
	expect_test["Moved"] = [ "@/CCC/test.file" ];
        vscript_test_wc__confirm_wii(expect_test);

	// do dirty update to AAA where neither CCC nor test.file exist.

	vscript_test_wc__update( cset["AAA"] );

	print(sg.exec(vv,"status","--verbose").stdout);

	var expect_test = new Array;
	expect_test["Added (Update)"]                = [ "@/CCC", "@/CCC/test.file" ];
	expect_test["Unresolved"]                    = [ "@/CCC", "@/CCC/test.file" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/CCC", "@/CCC/test.file" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to update forward to BBB...");

	vscript_test_wc__update( cset["BBB"] );
	print(sg.exec(vv,"status","--verbose").stdout);

	var expect_test = new Array;
	expect_test["Added (Update)"]                = [ "@/CCC", "@/CCC/test.file" ];
	expect_test["Unresolved"]                    = [ "@/CCC", "@/CCC/test.file" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/CCC", "@/CCC/test.file" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try to update forward CCC...");

	vscript_test_wc__update__expect_error( cset["CCC"],
					       "Conflicts or changes in the working copy prevented UPDATE: '@/CCC/test.file'" );

    }
}
