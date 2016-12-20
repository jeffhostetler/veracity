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

function st_portability_05()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    // this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.test_0 = function()
    {
	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	this.do_commit("Added AAA");

	vscript_test_wc__rename("AAA", "XXX");
	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/XXX" ];
	expect_test["Added"]   = [ "@/AAA" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// a REVERT-ITEM on "XXX" ("XXX" ==> "AAA") will
	// collide with the new "AAA" on all platforms.

	print("");
	print("================================================================");
	print("Trying revert-item...");
	print("");

	vscript_test_wc__revert_items__expect_error( [ "XXX" ],
						     "An item will interfere with revert");

	// nothing should have changed on disk
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("================================================================");
	print("Trying revert --all...");
	print("");

	vscript_test_wc__revert_all__expect_error( "trying revert-all",
						   "An item will interfere with revert" );

	// nothing should have changed on disk
        vscript_test_wc__confirm_wii(expect_test);

    }

    this.test_1 = function()
    {
	this.do_fsobj_mkdir("BBB");
	vscript_test_wc__addremove();
	this.do_commit("Added BBB");

	vscript_test_wc__rename("BBB", "YYY");
	this.do_fsobj_mkdir("bbb");
	vscript_test_wc__addremove();

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/YYY" ];
	expect_test["Added"]   = [ "@/bbb" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// a REVERT-ITEMS on "YYY" ("YYY" ==> "BBB") will
	// collide with the new "bbb" on Mac/Windows.  so we
	// should get a portability warning on all platforms.

	print("");
	print("================================================================");
	print("Trying revert-items...");
	print("");

	vscript_test_wc__revert_items__expect_error( [ "YYY" ],
						     "An item will interfere with revert");

	// nothing should have changed on disk
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("================================================================");
	print("Trying revert --all...");
	print("");

	vscript_test_wc__revert_all__expect_error( "trying revert-all",
						   "Portability conflict" );

	// nothing should have changed on disk
        vscript_test_wc__confirm_wii(expect_test);

    }

}
