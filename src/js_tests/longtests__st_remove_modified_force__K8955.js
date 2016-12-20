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

function st_remove_modified_force__K8955()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	var cover_symlinks = ((sg.platform() == "LINUX") || (sg.platform() == "MAC"));

	this.force_clean_WD();

        this.do_fsobj_create_file("file_1");
        this.do_fsobj_create_file("file_2");
        this.do_fsobj_create_file("file_3");
        this.do_fsobj_create_file("file_4");
        this.do_fsobj_create_file("file_5");
	if (cover_symlinks)
	{
	    this.do_make_symlink("file_1", "link_1");
	}
	vscript_test_wc__addremove();
	this.do_commit("add initial files");
	var cset_base = sg.wc.parents()[0];

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Make some dirt....");
	print("");

	this.do_fsobj_append_file("file_3", "Hello!");
	if (cover_symlinks)
	{
	    // retarget the symlink
	    this.do_make_symlink("file_2", "link_1");
	}

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_3" ];
	if (cover_symlinks)
	    expect_test["Modified"].push( "@/link_1" );
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Verify that normal 'vv remove' fails with unsafe...");
	print("");

	var result = sg.exec(vv, "remove", "file_3");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status != 0), "'vv remove file_3' should fail" );
	testlib.ok( (result.stderr.indexOf("The object cannot be safely removed") != -1), "Expect failure for file_3" );

	if (cover_symlinks)
	{
	    var result = sg.exec(vv, "remove", "link_1");
	    print(dumpObj(result, "sg.exec()", "", 0));
	    testlib.ok( (result.exit_status != 0), "'vv remove link_1' should fail" );
	    testlib.ok( (result.stderr.indexOf("The object cannot be safely removed") != -1), "Expect failure for link_1" );
	}

	// nothing should have changed

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_3" ];
	if (cover_symlinks)
	    expect_test["Modified"].push( "@/link_1" );
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Force the removes with -F...");
	print("");

	var backup_file_3 = vscript_test_wc__backup_name_of_file( "file_3", "sg00" );

	var result = sg.exec(vv, "remove", "-F", "file_3");
	print(dumpObj(result, "sg.exec()", "", 0));
	testlib.ok( (result.exit_status == 0), "'vv remove -F file_3' should work" );

	if (cover_symlinks)
	{
	    var result = sg.exec(vv, "remove", "-F", "link_1");
	    print(dumpObj(result, "sg.exec()", "", 0));
	    testlib.ok( (result.exit_status == 0), "'vv remove -F link_1' should work" );
	}

	// with V00115 'vv rm -F' creates backups for dirty files/symlinks

        var expect_test = new Array;
	expect_test["Removed"] = [ "@b/file_3" ];
	if (cover_symlinks)
	    expect_test["Removed"].push( "@b/link_1" );
	expect_test["Found"] = [ "@/" + backup_file_3 ];
        vscript_test_wc__confirm_wii(expect_test);

	// commit this so that we get a clean status for subsequent tests

	this.do_fsobj_remove_file( backup_file_3 );
	this.do_commit("commit changes to file_3");

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Verify that normal JS remove fails with unsafe...");
	print("");

	// make file_4 dirty and try to REMOVE it.

	this.do_fsobj_append_file("file_4", "Hello!");

	var expect_msg = "The object cannot be safely removed";
	try
	{
	    vscript_test_wc__remove( "file_4" );
	    testlib.ok( (false), "REMOVE should have thrown error: '" + expect_msg + "'" );
	}
	catch (e)
	{
	    this.my_verify_expected_throw(e, expect_msg);
	}

	// nothing should have changed

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_4" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Force the JS removes with -F...");
	print("");

	var backup_file_4 = vscript_test_wc__backup_name_of_file( "file_4", "sg00" );

	vscript_test_wc__remove_np( { "src" : "file_4", "force" : true } );

	// with V00115 'vv rm -F' creates backups for dirty files/symlinks
	// In the WC version, we never backup symlinks.

        var expect_test = new Array;
	expect_test["Removed"] = [ "@b/file_4" ];
	expect_test["Found"] = [ "@/" + backup_file_4 ];
        vscript_test_wc__confirm_wii(expect_test);

	// commit this so that we get a clean status for subsequent tests

	this.do_fsobj_remove_file( backup_file_4 );
	this.do_commit("commit changes to file_4");

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Verify that normal JS remove fails with unsafe...");
	print("");

	// make file_5 dirty and try to REMOVE it.

	this.do_fsobj_append_file("file_5", "Hello!");

	var expect_msg = "The object cannot be safely removed";
	try
	{
	    vscript_test_wc__remove( "file_5" );
	    testlib.ok( (false), "REMOVE should have thrown error: '" + expect_msg + "'" );
	}
	catch (e)
	{
	    this.my_verify_expected_throw(e, expect_msg);
	}

	// nothing should have changed

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_5" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("Force the JS removes with 'force' and 'no-backups' ...");
	print("");

	vscript_test_wc__remove_np( { "src" : "file_5",
				      "force" : true,
				      "no_backups" : true } );

	// with V00115 'vv rm -F' creates backups for dirty files/symlinks
	// with W6069  'vv rm -F --no-backups' doesn't

        var expect_test = new Array;
	expect_test["Removed"] = [ "@b/file_5" ];
        vscript_test_wc__confirm_wii(expect_test);

    }

}
