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

function st_portability_04()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    // this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.do_fsobj_mkdir("AAA");
	this.do_fsobj_mkdir("BBB");
	this.do_fsobj_mkdir("CCC");
	this.do_fsobj_mkdir("DDD");
	this.do_fsobj_mkdir("EEE");
	this.do_fsobj_mkdir("FFF");
	this.do_fsobj_mkdir("GGG");
	this.do_fsobj_mkdir("HHH");
	this.do_fsobj_mkdir("JJJ");

	this.do_fsobj_mkdir("WWW");
	this.do_fsobj_mkdir("XXX");
	this.do_fsobj_mkdir("YYY");
	this.do_fsobj_mkdir("ZZZ");
	vscript_test_wc__addremove();
	this.do_commit("Added Stuff");

	//////////////////////////////////////////////////////////////////

	// In release builds, only the error message is printed. So that's what we have to look for.
	var errmsgs = new Array();
	errmsgs["SG_ERR_INVALIDARG"]                        = "Invalid argument";
	errmsgs["SG_ERR_NOT_FOUND"]                         = "object could not be found";
	errmsgs["SG_ERR_PENDINGTREE_OBJECT_ALREADY_EXISTS"] = "The file/folder already exists";
	errmsgs["SG_ERR_WC_PORT_FLAGS"]                     = "Portability conflict";
	errmsgs["SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL"]    = "Attempt to operate on an item which is not under version control";
	errmsgs["SG_ERR_NO_EFFECT"]                         = "No effect";

	//////////////////////////////////////////////////////////////////

	print("Attempting 'rename AAA WWW'...");
	var r1 = sg.exec(vv, "rename", "AAA", "WWW");		// hard final collision ("WWW" already exists)
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);
	testlib.ok( ((r1.exit_status != 0) && (r1.stderr.search(errmsgs["SG_ERR_PENDINGTREE_OBJECT_ALREADY_EXISTS"]) >= 0)),
		    ("Expecting error containing '" + errmsgs["SG_ERR_PENDINGTREE_OBJECT_ALREADY_EXISTS"] + "'") );

	//////////////////////////////////////////////////////////////////

	print("Attempting 'rename BBB xxx'...");
	var r1 = sg.exec(vv, "rename", "BBB", "xxx");		// potential final collision ("xxx" potentially collides with "XXX")
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( ((r1.exit_status != 0) && (r1.stderr.search(errmsgs["SG_ERR_WC_PORT_FLAGS"]) >= 0)),
		    ("Expecting error containing '" + errmsgs["SG_ERR_WC_PORT_FLAGS"] + "'") );
	
	//////////////////////////////////////////////////////////////////

	print("Attempting 'rename ccc YYY'...");
	var r1 = sg.exec(vv, "rename", "ccc", "YYY");		// "ccc" isn't under version control ("CCC" is)
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( ((r1.exit_status != 0) && (r1.stderr.search(errmsgs["SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL"]) >= 0)),
		    ("Expecting error containing '" + errmsgs["SG_ERR_ITEM_NOT_UNDER_VERSION_CONTROL"] + "'") );
	
	//////////////////////////////////////////////////////////////////

	print("Attempting 'rename DDD ddd'...");
	var r1 = sg.exec(vv, "rename", "DDD", "ddd");		// potential transient and final collision (W6834)
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( (r1.exit_status == 0),
		    ("Expected rename to succeed using 2-step rename") );

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/ddd" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////

	print("Attempting 'rename EEE COM1'...");		// see W4000.
	var r1 = sg.exec(vv, "rename", "EEE", "COM1");		// "COM1" is a reserved name on Windows
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( ((r1.exit_status != 0) && (r1.stderr.search(errmsgs["SG_ERR_WC_PORT_FLAGS"]) >= 0)),
		    ("Expecting error containing '" + errmsgs["SG_ERR_WC_PORT_FLAGS"] + "'") );
	
	//////////////////////////////////////////////////////////////////

	print("Attempting 'rename FFF ZZZ/fff'...");
	var r1 = sg.exec(vv, "rename", "FFF", "ZZZ/fff");	// invalid arg -- destination must be entry only
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( ((r1.exit_status != 0) && (r1.stderr.search(errmsgs["SG_ERR_INVALIDARG"]) >= 0)),
		    ("Expecting error containing '" + errmsgs["SG_ERR_INVALIDARG"] + "'") );
	
	//////////////////////////////////////////////////////////////////

	print("Attempting 'rename GGG GGG'...");
	var r1 = sg.exec(vv, "rename", "GGG", "GGG");	// no effect -- was invalid arg -- src = dest
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( ((r1.exit_status != 0) && (r1.stderr.search(errmsgs["SG_ERR_NO_EFFECT"]) >= 0)),
		    ("Expecting error containing '" + errmsgs["SG_ERR_NO_EFFECT"] + "'") );
	
	//////////////////////////////////////////////////////////////////

	print("Attempting 'fs.rename HHH HHH_NEW'...");
	sg.fs.move("HHH", "HHH_NEW");

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/ddd" ];
	expect_test["Lost"] = [ "@/HHH" ];
	expect_test["Found"] = [ "@/HHH_NEW" ];
        vscript_test_wc__confirm_wii(expect_test);

	print("Attempting 'rename HHH HHH_NEW'...");
	var r1 = sg.exec(vv, "rename", "HHH", "HHH_NEW");	// after-the-fact rename
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( (r1.exit_status == 0),
		    ("Expected after-the-fact rename to succeed") );

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/ddd",
				   "@/HHH_NEW" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////

	print("Attempting 'fs.rename JJJ JJJ_NEW_1'...");
	sg.fs.move("JJJ", "JJJ_NEW_1");

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/ddd",
				   "@/HHH_NEW" ];
	expect_test["Lost"] = [ "@/JJJ" ];
	expect_test["Found"] = [ "@/JJJ_NEW_1" ];
        vscript_test_wc__confirm_wii(expect_test);

	print("Attempting 'rename JJJ JJJ_NEW_2'...");
	var r1 = sg.exec(vv, "rename", "JJJ", "JJJ_NEW_2");	// fail after-the-fact rename
	print("EXIT_STATUS: " + r1.exit_status);
	print("STDOUT:\n" + r1.stdout);
	print("STDERR:\n" + r1.stderr);

	testlib.ok( ((r1.exit_status != 0) && (r1.stderr.search(errmsgs["SG_ERR_NOT_FOUND"]) >= 0)),
		    ("Expecting error containing '" + errmsgs["SG_ERR_NOT_FOUND"] + "'") );

	var expect_test = new Array;
	expect_test["Renamed"] = [ "@/ddd",
				   "@/HHH_NEW" ];
	expect_test["Lost"] = [ "@/JJJ" ];
	expect_test["Found"] = [ "@/JJJ_NEW_1" ];
        vscript_test_wc__confirm_wii(expect_test);

    }

}
