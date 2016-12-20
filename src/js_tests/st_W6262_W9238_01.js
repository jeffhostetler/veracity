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

/////////////////////////////////////////////////////////////////////////////////////

var globalThis;  // var to hold "this" pointers

function st_W0901_W3903() 
{
    this.my_group = "st_W0901_W3903";	// this variable must match the above group name.
    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

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

    this.my_commit = function(name)
    {
	print("================================");
	print("Attempting COMMIT for: " + name);
	print("");

	sg.exec(vv, "branch", "attach", "master");
	this.my_names[name] = vscript_test_wc__commit(name);
    }

    this.my_update = function(name)
    {
	print("================================");
	print("Attempting UPDATE to: " + name);
	print("");

	vscript_test_wc__update(this.my_names[name]);
    }

    this.my_merge = function(name)
    {
	print("================================");
	print("Attempting MERGE with: " + name);
	print("");

	vscript_test_wc__merge_np( { "rev" : this.my_names[name] } );
    }

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
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

	this.my_make_wd();
	sg.vv2.init_new_repo( { "repo" : this.repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(this.repoName);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("X0 creating file");
	// We do this with 2 sets of items in 2 ways so that we can test RESOLVE taking
	// both the "baseline" and "other" choices each way.

	vscript_test_wc__write_file("file_1.txt", content_equal);
	vscript_test_wc__write_file("file_2.txt", content_equal);
	vscript_test_wc__write_file("file_3.txt", content_equal);
	vscript_test_wc__write_file("file_4.txt", content_equal);
        vscript_test_wc__addremove();
        this.my_commit("X0");

	vscript_test_wc__write_file("file_1.txt", (content_B1 + content_equal));
	vscript_test_wc__write_file("file_2.txt", (content_B1 + content_equal));
	vscript_test_wc__remove( "file_3.txt" );
	vscript_test_wc__remove( "file_4.txt" );
        this.my_commit("X1");

	// backup to X0 and create some conflicts

	this.my_update("X0");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__remove( "file_1.txt" );
	vscript_test_wc__remove( "file_2.txt" );
	vscript_test_wc__write_file("file_3.txt", (content_C1 + content_equal));
	vscript_test_wc__write_file("file_4.txt", (content_C1 + content_equal));
        this.my_commit("X2");

	// do regular merge (fold X1 into X2 with ancestor X0)

	this.my_merge("X1");

	// items were deleted in one side and changed in the other,
	// so the merge will re-add them.

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        @/file_1.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Unresolved
	// Choices:     Resolve this conflict by accepting a value from below by label.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: exists
	// 
	// Item:        @/file_2.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Unresolved
	// Choices:     Resolve this conflict by accepting a value from below by label.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: exists
	// 
	// Item:        @/file_3.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Unresolved
	// Choices:     Resolve this conflict by accepting a value from below by label.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: exists
	// 
	// Item:        @/file_4.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Unresolved
	// Choices:     Resolve this conflict by accepting a value from below by label.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: exists

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/file_1.txt", "@/file_2.txt" ];
	expect_test["Unresolved"]                    = [ "@/file_1.txt", "@/file_2.txt",
							 "@/file_3.txt", "@/file_4.txt" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/file_1.txt", "@/file_2.txt",
							 "@/file_3.txt", "@/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Dumping ISSUES and RESOLVE INFO (1)");

	var statusArray = sg.wc.status();
	for (var k in statusArray)
	{
	    if (statusArray[k].status.isResolved || statusArray[k].status.isUnresolved)
	    {
		var issue_k   = sg.wc.get_item_issues( { "src" : statusArray[k].path } );
		var resolve_k = sg.wc.get_item_resolve_info( { "src" : statusArray[k].path } );

		print(dumpObj(issue_k, "Issues for item: " + statusArray[k].path, "", 0));
		if (resolve_k != undefined)
		    print(dumpObj(resolve_k, "ResolveInfo for item: " + statusArray[k].path, "", 0));
	    }
	}

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE(1)");

	// here "baseline" refers to X2.
	// and  "other"    refers to X1.
	vscript_test_wc__resolve__accept("baseline", "@c/file_1.txt");	// this will delete it
	vscript_test_wc__resolve__accept("other",    "@c/file_2.txt");	// this will keep it (and in the X1 location)
	vscript_test_wc__resolve__accept("baseline", "@b/file_3.txt");	// this will keep it (and in the X2 location)
	vscript_test_wc__resolve__accept("other",    "@b/file_4.txt");	// this will delete it

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        (@c/file_1.txt)
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted baseline)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: deleted
	// 
	// Item:        @/file_2.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted other)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: exists
	// 
	// Item:        @/file_3.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted baseline)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: exists
	// 
	// Item:        (@b/file_4.txt)
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted other)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: deleted

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@c/file_1.txt", "@/file_2.txt" ];
	expect_test["Removed"]       = [ "@c/file_1.txt", "@b/file_4.txt" ];
	expect_test["Resolved"]                    = [ "@c/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@b/file_4.txt" ];
	expect_test["Choice Resolved (Existence)"] = [ "@c/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@b/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Dumping ISSUES and RESOLVE INFO (2)");

	var statusArray = sg.wc.status();
	for (var k in statusArray)
	{
	    if (statusArray[k].status.isResolved || statusArray[k].status.isUnresolved)
	    {
		var issue_k   = sg.wc.get_item_issues( { "src" : statusArray[k].path } );
		var resolve_k = sg.wc.get_item_resolve_info( { "src" : statusArray[k].path } );

		print(dumpObj(issue_k, "Issues for item: " + statusArray[k].path, "", 0));
		if (resolve_k != undefined)
		    print(dumpObj(resolve_k, "ResolveInfo for item: " + statusArray[k].path, "", 0));
	    }
	}

	//////////////////////////////////////////////////////////////////
	// W0901 addresses whether we can re-resovle a deleted item and get
	// it restored properly.  Let's flip-flop all of our choices.
	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Begin flip-flop on delete-vs-* resolutions.");

	// here "baseline" refers to X2.
	// and  "other"    refers to X1.
	vscript_test_wc__resolve__accept("other",    "@c/file_1.txt", "--overwrite");	// restore deleted item
	vscript_test_wc__resolve__accept("baseline", "@c/file_2.txt", "--overwrite");	// delete kept item
	vscript_test_wc__resolve__accept("other",    "@b/file_3.txt", "--overwrite");	// delete kept item
	vscript_test_wc__resolve__accept("baseline", "@b/file_4.txt", "--overwrite");	// restore deleted item

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        @/file_1.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted other)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: exists
	// 
	// Item:        (@c/file_2.txt)
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted baseline)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: deleted
	// 
	// Item:        (@b/file_3.txt)
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted other)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: deleted
	// 
	// Item:        @/file_4.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted baseline)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: exists

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/file_1.txt", "@c/file_2.txt" ];
	expect_test["Removed"]       = [ "@c/file_2.txt", "@b/file_3.txt" ];
	expect_test["Resolved"]                    = [ "@/file_1.txt", "@c/file_2.txt", "@b/file_3.txt", "@/file_4.txt" ];
	expect_test["Choice Resolved (Existence)"] = [ "@/file_1.txt", "@c/file_2.txt", "@b/file_3.txt", "@/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("flip-flop again to restore all.");

	// here "baseline" refers to X2.
	// and  "other"    refers to X1.
	vscript_test_wc__resolve__accept("other",    "@c/file_2.txt", "--overwrite");	// restore deleted item
	vscript_test_wc__resolve__accept("baseline", "@b/file_3.txt", "--overwrite");	// restore deleted item

	vscript_test_wc__resolve__list_all_verbose();
	// TODO

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/file_1.txt", "@/file_2.txt"  ];
	expect_test["Resolved"]                    = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	expect_test["Choice Resolved (Existence)"] = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Make some dirt in each file");

	vscript_test_wc__write_file("file_1.txt", content_equal + content_W1);
	vscript_test_wc__write_file("file_2.txt", content_equal + content_W1);
	vscript_test_wc__write_file("file_3.txt", content_equal + content_W1);
	vscript_test_wc__write_file("file_4.txt", content_equal + content_W1);

	vscript_test_wc__resolve__list_all_verbose();
	// TODO

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/file_1.txt", "@/file_2.txt" ];
	expect_test["Modified"]      = [ "@/file_3.txt", "@/file_4.txt" ]; // file_[12] won't show up because don't do ADDED+MODIFIED
	expect_test["Resolved"]                    = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	expect_test["Choice Resolved (Existence)"] = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Re-resolve each to working.");

	vscript_test_wc__resolve__accept("working",  "@c/file_1.txt", "--overwrite");	// accept modified item as is
	vscript_test_wc__resolve__accept("working",  "@c/file_2.txt", "--overwrite");	// accept modified item as is
	vscript_test_wc__resolve__accept("working",  "@b/file_3.txt", "--overwrite");	// accept modified item as is
	vscript_test_wc__resolve__accept("working",  "@b/file_4.txt", "--overwrite");	// accept modified item as is

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        @/file_1.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted working)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: exists
	// 
	// Item:        @/file_2.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted working)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: deleted
	//       other: exists
	//     working: exists
	// 
	// Item:        @/file_3.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted working)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: exists
	// 
	// Item:        @/file_4.txt
	// Conflict:    Existence
	//              You must choose if the item should exist.
	// Cause:       Delete/Edit
	//              File deleted in one branch and changed in another.
	// Status:      Resolved (accepted working)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: exists
	//    baseline: exists
	//       other: deleted
	//     working: exists

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/file_1.txt", "@/file_2.txt" ];
	expect_test["Modified"]      = [ "@/file_3.txt", "@/file_4.txt" ]; // file_[12] won't show up because don't do ADDED+MODIFIED
	expect_test["Resolved"]                    = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	expect_test["Choice Resolved (Existence)"] = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Re-resolve each by picking baseline/other and delete the files.");

	// note that each of the files are dirty.  but because we are only
	// settling the question of "existence", we may not look at the contents
	// of the files first.
	//
	// TODO 2012/06/12 See W9238.  Shouldn't these fail because the delete
	// TODO                        should want to complain about them being dirty?
	// TODO                        Currently _accept__existence() does an implicit --force --no-backups
	// TODO                        and I'm not sure it should.

	vscript_test_wc__resolve__accept("baseline", "@c/file_1.txt", "--overwrite");
	vscript_test_wc__resolve__accept("baseline", "@c/file_2.txt", "--overwrite");
	vscript_test_wc__resolve__accept("other",    "@b/file_3.txt", "--overwrite");
	vscript_test_wc__resolve__accept("other",    "@b/file_4.txt", "--overwrite");

	vscript_test_wc__resolve__list_all_verbose();

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@c/file_1.txt", "@c/file_2.txt"  ];
	expect_test["Removed"]       = [ "@c/file_1.txt", "@c/file_2.txt", "@b/file_3.txt", "@b/file_4.txt" ];
	expect_test["Resolved"]                    = [ "@c/file_1.txt", "@c/file_2.txt", "@b/file_3.txt", "@b/file_4.txt" ];
	expect_test["Choice Resolved (Existence)"] = [ "@c/file_1.txt", "@c/file_2.txt", "@b/file_3.txt", "@b/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Re-resolve each by picking baseline/other and keep the files.");

	// note that each of the files where dirty before the delete. 
	// but because we are only settling the question of "existence",
	// these will fetch the official reference versions of each.

	vscript_test_wc__resolve__accept("other",    "@c/file_1.txt", "--overwrite");
	vscript_test_wc__resolve__accept("other",    "@c/file_2.txt", "--overwrite");
	vscript_test_wc__resolve__accept("baseline", "@b/file_3.txt", "--overwrite");
	vscript_test_wc__resolve__accept("baseline", "@b/file_4.txt", "--overwrite");

	vscript_test_wc__resolve__list_all_verbose();

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/file_1.txt", "@/file_2.txt"  ];
	expect_test["Resolved"]                    = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	expect_test["Choice Resolved (Existence)"] = [ "@/file_1.txt", "@/file_2.txt", "@/file_3.txt", "@/file_4.txt" ];
	vscript_test_wc__confirm_wii(expect_test);


    }
}
