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

function st_attrbits_W3801_02()
{
    this.my_group = "st_attrbits_W3801_02";	// this variable must match the above group name.

    this.my_data_dir = sg.fs.getcwd();	// we start in the .../src/js_tests directory.
    this.my_fi = this.my_data_dir + "/data__" + this.my_group + ".fi";

    this.my_fi_test1 = "../" + this.my_group + "_test1.fi";
    this.my_fi_test2 = "../" + this.my_group + "_test2.fi";

    this.no_setup = true;		// do not create an initial REPO and WD.

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    this.unique = this.my_group + "_" + new Date().getTime();

    this.repoName = this.unique;
    this.repoNameDup1 = this.repoName + "_dup";
    this.repoNameDup2 = this.repoName + "_dup2";

    this.rootDir = pathCombine(tempDir, this.unique);
    this.wdDirs = [];
    this.nrWD = 0;
    this.my_names = new Array;

    this.my_make_wd = function()
    {
	vscript_test_wc__print_section_divider("WD[" + this.nrWD + "]");
	var new_wd = pathCombine(this.rootDir, "wd_" + this.nrWD);

	print("Creating WD[" + this.nrWD + "]: " + new_wd);
	print("");

	this.do_fsobj_mkdir_recursive( new_wd );
	this.do_fsobj_cd( new_wd );
	this.wdDirs.push( new_wd );
	
	this.nrWD += 1;

	return new_wd;
    }

    this.my_test_it = function( local_repoName )
    {
	// Do the various tests that we want to do with in the
	// current WD with the given args.  (And isolate whether
	// we are running on a manually-create or fast-import repo.

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	vscript_test_wc__print_section_divider("Running tests using repo='"+local_repoName+"'.");
	var tags = sg.vv2.tags({"repo":local_repoName});
	print(sg.to_json__pretty_print(tags));

	for (t in tags)
	{
	    print("Treenodes of " + tags[t].tag + " " + tags[t].csid);
	    var r = sg.exec(vv, "dump_treenode", "--repo="+local_repoName, tags[t].csid);
	    print(r.stdout);
	    print();
	}

	//////////////////////////////////////////////////////////////////
	// Confirm that the CSets are created as expected.
	// That is, did we get all the attrbits set correctly
	// from the fast-export/fast-import.

	this.my_make_wd();

	vscript_test_wc__print_section_divider("Checkout cset_base for eval");
	sg.wc.checkout( { "repo" : local_repoName,
			  "path" : ".",
			  "tag"  : "cset_base" } );

        var expect_test = new Array;
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );
	
	var s = sg.wc.status({"src":"@/file_xoff", "list_unchanged":true});
	print("Verify details of file_xoff...");
//	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 0), "Baseline should have 0600");
	testlib.ok( (s[0].WC.attributes == 0), "Working should have 0600 in theory");

	var s = sg.wc.status({"src":"@/file_xon", "list_unchanged":true});
	print("Verify details of file_xon...");
//	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 1), "Baseline should have 0700");
	testlib.ok( (s[0].WC.attributes == 1), "Working should have 0700 in theory");


	this.my_make_wd();

	vscript_test_wc__print_section_divider("Checkout L1_a for eval");
	sg.wc.checkout( { "repo" : local_repoName,
			  "path" : ".",
			  "tag"  : "cset_L1_a" } );

        var expect_test = new Array;
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );
	
	var s = sg.wc.status({"src":"@/file_xoff", "list_unchanged":true});
	print("Verify details of file_xoff...");
//	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 1), "Baseline should have 0700");
	testlib.ok( (s[0].WC.attributes == 1), "Working should have 0700 in theory");

	var s = sg.wc.status({"src":"@/file_xon", "list_unchanged":true});
	print("Verify details of file_xon...");
//	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 0), "Baseline should have 0600");
	testlib.ok( (s[0].WC.attributes == 0), "Working should have 0600 in theory");


	this.my_make_wd();

	vscript_test_wc__print_section_divider("Checkout L0_a for eval");
	sg.wc.checkout( { "repo" : local_repoName,
			  "path" : ".",
			  "tag"  : "cset_L0_a" } );

        var expect_test = new Array;
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "list_sparse" : true } ) );
	
	var s = sg.wc.status({"src":"@/file_xoff", "list_unchanged":true});
	print("Verify details of file_xoff...");
//	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 0), "Baseline should have 0600");
	testlib.ok( (s[0].WC.attributes == 0), "Working should have 0600 in theory");

	var s = sg.wc.status({"src":"@/file_xon", "list_unchanged":true});
	print("Verify details of file_xon...");
//	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 1), "Baseline should have 0700");
	testlib.ok( (s[0].WC.attributes == 1), "Working should have 0700 in theory");


	//////////////////////////////////////////////////////////////////
	// Print contents of L0_a

	vscript_test_wc__print_section_divider("L0_a WD before merge...");
	print(sg.to_json__pretty_print(sg.wc.status({"list_sparse":true,
						     "list_unchanged":true,
						     "no_ignores":true})));


	//////////////////////////////////////////////////////////////////
	// Starting with baseline L0_a merge in L1_a

	vscript_test_wc__print_section_divider("Merge in L1_a...");
	var result = sg.wc.merge( { "tag" : "cset_L1_a",
				    "verbose" : true } );
	print(sg.to_json__pretty_print(result));
	// the merge journal should report that we had to toggle 
	// the attrbits on both files.
	nrEntries = 0
	for (var j in result.journal)
	{
	    print("Journal entry for: " + result.journal[j].src);
	    if (result.journal[j].src == "@/file_xoff")
	    {
		testlib.ok( (result.journal[j].op == "overwrite_file_from_repo"), "Expect 'overwrite' for file_xoff");
		testlib.ok( (result.journal[j].attrbits == 1), "Expect 'attrbits=1' for file_xoff");
		nrEntries++;
	    }
	    else if (result.journal[j].src == "@/file_xon")
	    {
		testlib.ok( (result.journal[j].op == "overwrite_file_from_repo"), "Expect 'overwrite' for file_xon");
		testlib.ok( (result.journal[j].attrbits == 0), "Expect 'attrbits=0' for file_xon");
		nrEntries++;
	    }
	    else if (result.journal[j].src == "@/noise_L1_a")
	    {
		nrEntries++;
	    }
	    else
	    {
		testlib.ok( (0), "Unexpected journal entry: " + result.journal[j].src);
	    }
	}
	testlib.ok( (nrEntries == 3), "Expect 3 journal entries");


	print("");
	print("WD after merge (L0_a, L1_a) ...");
	print(sg.to_json__pretty_print(sg.wc.status({"list_sparse":true,
						     "list_unchanged":true,
						     "no_ignores":true})));


        var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/noise_L1_a" ];
	expect_test["Modified"]      = [ "@/file_xoff", "@/file_xon" ];
	expect_test["Attributes"]    = [ "@/file_xoff", "@/file_xon" ];
        vscript_test_wc__confirm(expect_test,
				 sg.wc.status( { "no_ignores" : true, 
						 "list_sparse" : true } ) );


	print("WD after merge (L0_a, L1_a) mstatus ...");
	print(sg.to_json__pretty_print(sg.wc.mstatus({"no_ignores":true})));
        var expect_test = new Array;
	expect_test["Attributes (WC!=A,WC!=B,WC==C)"] = [ "@/file_xoff", "@/file_xon" ];
	expect_test["Modified (WC!=A,WC!=B,WC==C)"] = [ "@/file_xoff", "@/file_xon" ];
	expect_test["Added (B)"] = [ "@/noise_L0_a" ];
	expect_test["Added (C)"] = [ "@/noise_L1_a" ];
	vscript_test_wc__mstatus_confirm_wii(expect_test);


	var s = sg.wc.status({"src":"@/file_xoff", "list_unchanged":true});
	print("Verify post-merge details of file_xoff...");
	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 0),  "Baseline should have 0600");
	testlib.ok( (s[0].WC.attributes == 1), "Working should have 0700 in theory");


	var s = sg.wc.status({"src":"@/file_xon", "list_unchanged":true});
	print("Verify post-merge details of file_xon...");
	print(sg.to_json__pretty_print(s));
	testlib.ok( (s[0].B.attributes == 1), "Baseline should have 0700");
	testlib.ok( (s[0].WC.attributes == 0), "Working should have 0600 in theory");


	print("Verify post-merge mstatus details...");
	var s = sg.wc.mstatus();
	for (var k in s)
	{
	    if (s[k].path == "@/file_xoff")
	    {
		testlib.ok( (s[k].A.attributes == 0), "Ancestor should have 0600" );
		testlib.ok( (s[k].B.attributes == 0), "Baseline should have 0600" );
		testlib.ok( (s[k].C.attributes == 1), "Baseline should have 0700" );
		testlib.ok( (s[k].WC.attributes == 1), "Working should have 0700 in theory" );
	    }
	    else if (s[k].path == "@/file_xon")
	    {
		testlib.ok( (s[k].A.attributes == 1), "Ancestor should have 0700" );
		testlib.ok( (s[k].B.attributes == 1), "Baseline should have 0700" );
		testlib.ok( (s[k].C.attributes == 0), "Other should have 0600" );
		testlib.ok( (s[k].WC.attributes == 0), "Working should have 0600 in theory" );
	    }
	}


	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

    }

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////
	// Setup a repo with known content.
	//
	// If on a Unix-based system, create a repo 2 heads and then fast-export it.
	// If on Windows, populate the repo using fast-import.
	// The idea is that once we get the test working on Unix, we'll checkin the .fi file
	// so that Windows can just use it in subsequent test runs.
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////

	var bHave_X_bit = ((sg.platform() == "LINUX") || (sg.platform() == "MAC"));
	if (bHave_X_bit)
	{
	    this.my_make_wd();
	    sg.vv2.init_new_repo( { "repo" : this.repoName,
				    "hash" : "SHA1/160",
				    "path" : "."
				  } );
	    whoami_testing(this.repoName);

            this.do_fsobj_create_file("file_1");
            this.do_fsobj_create_file("file_xoff");
	    this.do_chmod("file_xoff", 0600);
            this.do_fsobj_create_file("file_xon");
	    this.do_chmod("file_xon", 0700);
	    vscript_test_wc__addremove();
	    var cset_base = vscript_test_wc__commit_np( { "message" : "add initial files" } );
	    sg.vv2.add_tag( { "rev" : cset_base, "value" : "cset_base" } );

	    //////////////////
	    // create
	    //       _base
	    //      /     \
	    //  L0_a       L1_a
	    // make same content change in both branches

	    vscript_test_wc__update_attached(cset_base, "master");

            this.do_fsobj_create_file("noise_L0_a");
	    vscript_test_wc__addremove();
	    var cset_L0_a = vscript_test_wc__commit_np( { "message" : "L0_a changes" } );
	    sg.vv2.add_tag( { "rev" : cset_L0_a, "value" : "cset_L0_a" } );

	    vscript_test_wc__update_attached(cset_base, "master");

            this.do_fsobj_create_file("noise_L1_a");
	    this.do_fsobj_append_file("file_xoff", "hello again");
	    this.do_chmod("file_xoff", 0700);
	    this.do_fsobj_append_file("file_xon", "hello again");
	    this.do_chmod("file_xon",  0600);
	    vscript_test_wc__addremove();
	    var cset_L1_a = vscript_test_wc__commit_np( { "message" : "L1_a changes" } );
	    sg.vv2.add_tag( { "rev" : cset_L1_a, "value" : "cset_L1_a" } );

	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////
	    // WARNING: if you change anything in the above setup, run the test
	    // WARNING: on Linux/Mac and use the .fi file we generate below to
	    // WARNING: update the data__*.fi file we reference at the top and
	    // WARNING: check it in.
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////

	    // Fast-export what we just created.
	    vscript_test_wc__print_section_divider("Trying to fast-export repo '" + this.repoName + "'...");
	    var r = sg.exec(vv, "fast-export", this.repoName, this.my_fi_test1);
	    testlib.ok( (r.exit_status == 0), "Fast-Export to my_fi_test1.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));

	    // Fast-import the .fi we just created.
	    vscript_test_wc__print_section_divider("Trying to fast-import repo into '" + this.repoNameDup1 + "'...");
	    var r = sg.exec(vv, "fast-import", this.my_fi_test1, this.repoNameDup1);
	    testlib.ok( (r.exit_status == 0), "Fast-Import from my_fi_test1.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));

	    vscript_test_wc__print_section_divider("Trying to fast-export repo '" + this.repoNameDup1 + "'...");
	    var r = sg.exec(vv, "fast-export", this.repoNameDup1, this.my_fi_test2);
	    testlib.ok( (r.exit_status == 0), "Fast-Export to my_fi_test2.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));

	    // TODO 2012/12/18 we should be able to do some crude comparisons of my_fi_test1 and my_fi_test2....

	    // Fast-import the .fi we have under version control.
	    vscript_test_wc__print_section_divider("Trying to fast-import repo into '" + this.repoNameDup2 + "'...");
	    var r = sg.exec(vv, "fast-import", this.my_fi, this.repoNameDup2);
	    testlib.ok( (r.exit_status == 0), "Fast-Import from my_fi.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));

	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////

	    // Run tests on the repo we created directly.
	    this.my_test_it( this.repoName    );

	    if (0)
	    {
		// TODO 2013/02/04 I'm going to disable running the tests on the
		// TODO            generated .fi files because fast-export doesn't
		// TODO            correctly set the +x bit in the .fi file.
		// TODO
		// TODO            See W7617.
		this.my_test_it( this.repoNameDup1);
	    }

	    // Run the test on the repo created from the .fi that we 
	    // have under version control.  This .fi was hand-patched
	    // to get around W7617.
	    this.my_test_it( this.repoNameDup2);
	}
	else
	{
	    // Since Windows cannot create files with 'chmod +x', we skip actually building
	    // a new repo using a WD and testing the construction.  Just fast-import the .fi we have
	    // under version control.
	    vscript_test_wc__print_section_divider("Trying to fast-import repo into '" + this.repoNameDup2 + "'...");
	    var r = sg.exec(vv, "fast-import", this.my_fi, this.repoNameDup2);
	    testlib.ok( (r.exit_status == 0), "Fast-Import from my_fi.");
	    if (r.exit_status != 0)
		print(sg.to_json__pretty_print(r));
	    
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////
	    //////////////////////////////////////////////////////////////////

	    // Run the test on the repo created from the .fi that we 
	    // have under version control.  This .fi was hand-patched
	    // to get around W7617.
	    this.my_test_it( this.repoNameDup2 );
	}

	//////////////////////////////////////////////////////////////////
    }
}

