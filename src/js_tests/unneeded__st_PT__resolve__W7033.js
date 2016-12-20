// TODO 2012/06/05 This test is a back-port into "master" from the
// TODO            "PendingTreeStudy" branch.
// TODO
// TODO            We should be able to just delete it rather than
// TODO            re-port it.  See st_wc_W7420_W7033.js
//////////////////////////////////////////////////////////////////

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
// Basic MERGE tests that DO NOT require UPDATE nor REVERT.
//////////////////////////////////////////////////////////////////

function st_PT__resolve__W7033()
{
    var my_group = "st_PT__resolve__W7033";	// this variable must match the above group name.

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
	var unique = my_group + "_" + new Date().getTime();

	var repoName = unique;
	var rootDir = pathCombine(tempDir, unique);
	var wdDirs = [];
	var csets = {};
	var nrWD = 0;

	var my_make_wd = function(obj)
	{
	    var new_wd = pathCombine(rootDir, "wd_" + nrWD);

	    print("================================================================");
	    print("Creating WD[" + nrWD + "]: " + new_wd);
	    print("================================================================");

	    obj.do_fsobj_mkdir_recursive( new_wd );
	    obj.do_fsobj_cd( new_wd );
	    wdDirs.push( new_wd );
	    
	    nrWD += 1;

	    return new_wd;
	}

	var my_write_file = function(pathname, content)
	{
	    indentLine("=> write_file: " + pathname);

	    sg.file.write(pathname, content);
	}

	var my_commit = function(msg)
	{
	    indentLine("=> commit '" + msg + "'");
	    
	    return sg.pending_tree.commit( ["message="+msg] );
	}

	var my__resolve__list_all = function()
	{
	    indentLine("=> resolve list --all --verbose:");

	    var r = sg.exec(vv, "resolve", "list", "--all", "--verbose");

	    print(r.stdout);
	    print(r.stderr);

	    testlib.ok((r.exit_status == 0), "Exit status from RESOLVE LIST ALL.");
	}

	var my__resolve__is_resolved = function( file )
	{
	    indentLine("=> resolve list --all --verbose '" + file + "':");

	    var r = sg.exec(vv, "resolve", "list", "--all", "--verbose", file);

	    print(r.stdout);
	    print(r.stderr);

	    var regex = new RegExp("Status:[ \t]*Resolved");
	    var bIsResolved = regex.test( r.stdout );
	    
	    testlib.ok( (bIsResolved), "Expect '" + file + "' to be resolved." );
	}

	//////////////////////////////////////////////////////////////////
	// stock content for creating files.

	var content_equal = ("This is a test 1.\n"
			     +"This is a test 2.\n"
			     +"This is a test 3.\n"
			     +"This is a test 4.\n"
			     +"This is a test 5.\n"
			     +"This is a test 6.\n"
			     +"This is a test 7.\n"
			     +"This is a test 8.\n");
	var content_B1 = "abcdefghij\n";
	var content_C1 = "0123456789\n";
	var content_W1 = "qwertyuiop\n";

	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	my_make_wd(this);
	sg.vv.init_new_repo( [ "hash=SHA1/160" ], repoName );
	whoami_testing(repoName);

	//////////////////////////////////////////////////////////////////
	// Build a sequence of CSETs using current WD.

	my_write_file("file_0", content_equal);
	my_write_file("file_1", content_equal);
	my_write_file("file_2", content_equal);
	my_write_file("file_3", content_equal);
	my_write_file("file_4", content_equal);
	my_write_file("file_5", content_equal);
	my_write_file("file_6", content_equal);
	my_write_file("file_7", content_equal);
	my_write_file("file_8", content_equal);
	my_write_file("file_9", content_equal);
	this.do_addremove_scan();
	csets.A = my_commit("A");

	//////////////////////////////////////////////////////////////////
	// Start a sequence of CSETS in a new WD.

	my_make_wd(this);
	var o = sg.exec(vv, "checkout", repoName, ".", "--rev", csets.A, "--attach", "master");
	if (o.exit_status != 0)
	{
	    print(dumpObj(o,"checkout","",0));
	    testlib.ok( (0), "checkout");
	}
	// re-write the file contents so that they look edited.
	my_write_file("file_1", (content_B1 + content_equal));
	my_write_file("file_2", (content_B1 + content_equal));
	my_write_file("file_3", (content_B1 + content_equal));
	my_write_file("file_4", (content_B1 + content_equal));
	csets.B = my_commit("B");
		    
	//////////////////////////////////////////////////////////////////
	// Start an alternate sequence of CSETS in a new WD.

	my_make_wd(this);
	var o = sg.exec(vv, "checkout", repoName, ".", "--rev", csets.A, "--attach", "master");
	if (o.exit_status != 0)
	{
	    print(dumpObj(o,"checkout","",0));
	    testlib.ok( (0), "checkout");
	}
	// re-write the file contents so that they look edited.
	my_write_file("file_1", (content_equal + content_C1));
	my_write_file("file_2", (content_equal + content_C1));
	my_write_file("file_3", (content_equal + content_C1));
	my_write_file("file_4", (content_equal + content_C1));
	csets.C = my_commit("C");

	//////////////////////////////////////////////////////////////////
	// Now start some MERGES using a fresh WD for each.
	//
	// (We don't yet have UPDATE and REVERT working in
	// the WC version of the code.)
	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("Checkout B and merge C into it.");
	print("================================================================");

	my_make_wd(this);
	var o = sg.exec(vv, "checkout", repoName, ".", "--rev", csets.B, "--attach", "master");
	if (o.exit_status != 0)
	{
	    print(dumpObj(o,"checkout","",0));
	    testlib.ok( (0), "checkout");
	}

	var o = sg.exec(vv, "merge", "--rev", csets.C);

	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("RESOLVE LIST ALL (1)");
	print("================================================================");

	my__resolve__list_all();

	// using plain/traditional STATUS, we expect just the changes relative to B.
	//
	// Since B put unique content at the front and C put it at the end
	// of a large common area, we expect auto-merge to succeed.

	var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_1",
				    "@/file_2",
				    "@/file_3",
				    "@/file_4",
				  ];
        this.confirm_when_ignoring_ignores(expect_test);

	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("Creating dirt....");
	print("================================================================");

	// dirty the file post-merge and check with MSTATUS.
	// (again we just re-write them with the new content.)

	// edit the file to look like it did in L0 (aka take-left).
	my_write_file("file_1", (content_B1 + content_equal));

	// edit the file to look like it did in L1 (aka take-right).
	my_write_file("file_2", (content_equal + content_C1));

	// edit the file to look like it did in A (aka take-neither).
	my_write_file("file_3", (content_equal));

	// edit the file after the auto-merge.
	my_write_file("file_4", (content_B1 + content_equal + content_C1 + content_W1));

	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("RESOLVE LIST ALL (2)");
	print("================================================================");

	my__resolve__list_all();

	//////////////////////////////////////////////////////////////////
	print("================================================================");
	print("RESOLVE (3)");
	print("================================================================");

	my__resolve__is_resolved( "file_1" );
	my__resolve__is_resolved( "file_2" );
	my__resolve__is_resolved( "file_3" );
	my__resolve__is_resolved( "file_4" );

	//////////////////////////////////////////////////////////////////
	// I'm turning off the final commit test because it is just noise
	// here.  It passes because the pendingtree commit code only looks
	// as the resolved state field in the pvhIssues inside pendingtree.
	// (Which correctly has everything marked as resolved.
	//
	// Whereas the above __is_resolved() tests just test the saved
	// pvhResolve info.

	if (0)
	{
	    print("================================================================");
	    print("TRYING TO COMMIT AFTER MERGE");
	    print("================================================================");

	    var parentsBefore = sg.pending_tree.parents();
	    print(dumpObj(parentsBefore,"Parents Before", "", 0));
	    testlib.ok( (parentsBefore[0] == csets.B), "P0" );
	    testlib.ok( (parentsBefore[1] == csets.C), "P1" );

	    csets.D = my_commit("D");

	    var parentsAfter = sg.pending_tree.parents();
	    print(dumpObj(parentsAfter,"Parents After", "", 0));
	    testlib.ok( (parentsAfter[0] != csets.B), "P0" );
	}

    }

}
