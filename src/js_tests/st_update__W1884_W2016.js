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

function st_update__W1884_W2016()
{
    this.my_group = "st_update__W1884_W2016";	// this variable must match the above group name.
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
    
    this.my_partial_commit = function(name, path)
    {
	print("================================");
	print("Attempting PARTIAL COMMIT for: " + name + " with: " + path);
	print("");

	sg.exec(vv, "branch", "attach", "master");
	this.my_names[name] = vscript_test_wc__commit_np( { "message" : name, "src" : path } );
    }
    
    this.my_update = function(name)
    {
	print("================================");
	print("Attempting UPDATE to: " + name);
	print("");

	vscript_test_wc__update(this.my_names[name]);
    }

    this.my_update__expect_error = function(name, msg)
    {
	print("================================");
	print("Attempting UPDATE to: " + name);
	print("");

	vscript_test_wc__update__expect_error(this.my_names[name], msg);
    }

    this.my_find_unique_path = function(pattern)
    {
	// given simple pattern, such as "@/b3/a3*", find the
	// actual current pathname for the item (which might
	// be of the form "@/b3/a3/~g123456~") such that
	// the caller can pass it to a command that takes an
	// exact name.

	var re = new RegExp( pattern );
	var status = sg.wc.status();
	for (var s_index in status)
	    if (re.test(status[s_index].path))
		return (status[s_index].path);

	return; // undefined
    }

    this.my_random = function(limit)
    {
	// return a number between [0..limit-1]
	return (Math.floor(Math.random()*limit));
    }

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
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
	vscript_test_wc__print_section_divider("Testing dirty updates on edited files.");

	var X0_content = content_equal;
	vscript_test_wc__write_file("file_A.txt", X0_content);
	vscript_test_wc__addremove();
        this.my_commit("X0");

	var X1_content = (content_B1 + content_equal);
	vscript_test_wc__write_file("file_A.txt", X1_content);
        this.my_commit("X1");

	this.do_fsobj_mkdir("QQQ0");	// noise to ensure we get a new cset.
	vscript_test_wc__addremove();
        this.my_commit("X2");

	var X3_content = (content_B1 + content_equal + content_C1);
	vscript_test_wc__write_file("file_A.txt", X3_content);
        this.my_commit("X3");

	//////////////////////////////////////////////////////////////////
	// backup to X0 and create some dirt/conflicts and try to carry it forward.

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Creating dirt in X0.");

	this.my_update("X0");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);

	var X0_dirt0_content = (content_equal + content_W1);
	vscript_test_wc__write_file("file_A.txt", X0_dirt0_content);

	var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_A.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Dirty UPDATE to X1.");

	// W2016 asks if we have edited content and the file is also
	// modified in the update-target, then will UPDATE do an AUTO-MERGE.

	this.my_update("X1");

	var expect_test = new Array;
	expect_test["Modified"]    = [ "@/file_A.txt" ];
	expect_test["Auto-Merged"] = [ "@/file_A.txt" ];
	expect_test["Resolved"]    = [ "@/file_A.txt" ];
	expect_test["Choice Resolved (Contents)"]    = [ "@/file_A.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        @/file_A.txt
	// Conflict:    Contents
	//              You must choose/generate the item's contents.
	// Cause:       Edit/Edit
	//              Changes to item's contents in different branches conflict.
	// Status:      Resolved (accepted automerge)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: File's contents from the merge's common ancestor changeset
	//    baseline: File's contents from the original parent changeset
	//       other: File's contents from the merged-in changeset
	//   automerge: Merge of baseline and other using :merge from Jun 13 at 9:35
	//     working: File's current contents in the working copy (from automerge)

	// auto-merge should have created this:
	var X1_dirt1_content = (content_B1 + content_equal + content_W1);

	var auto_merged_content = sg.file.read("file_A.txt");
	testlib.ok( (auto_merged_content == X1_dirt1_content), "Auto-Merge on file_A.txt in X1.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Dirty UPDATE to X2.");

	// W2065 asks if we can do a DIRTY UPDATE with RESOLVED ISSUES.
	// W2065: If the item has resolved status, we can drop the issue
	// during the update (like commit would).
	//
	// Since X2 did not modify file_A.txt, an update to it should
	// silently retire it.  We still have the uncommitted and
	// modified content.

	this.my_update("X2");

	var expect_test = new Array;
	expect_test["Modified"]    = [ "@/file_A.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all_verbose();
	// Should be empty.

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Dirty UPDATE to X3.");

	// Let auto-merge try again (W2016) but this time it should get
	// an unresolved content conflict.

	this.my_update("X3");

	var expect_test = new Array;
	expect_test["Modified"]    = [ "@/file_A.txt" ];
	expect_test["Auto-Merged"] = [ "@/file_A.txt" ];
	expect_test["Unresolved"]  = [ "@/file_A.txt" ];
	expect_test["Choice Unresolved (Contents)"]  = [ "@/file_A.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        @/file_A.txt
	// Conflict:    Contents
	//              You must choose/generate the item's contents.
	// Cause:       Edit/Edit
	//              Changes to item's contents in different branches conflict.
	// Status:      Unresolved
	// Choices:     Resolve this conflict by accepting a value from below by label.
	//    ancestor: File's contents from the merge's common ancestor changeset
	//    baseline: File's contents from the original parent changeset
	//       other: File's contents from the merged-in changeset
	//   automerge: Merge of baseline and other using :merge from Jun 13 at 9:35
	//     working: File's current contents in the working copy (from automerge)

	//////////////////////////////////////////////////////////////////
	// W1884 asks if RESOLVE can handle "content" conflicts in an update-context.
	//////////////////////////////////////////////////////////////////

	// W3691 says we should name the choices differently for UPDATES than for MERGES.
	// TODO 2012/06/13 change these keys when we fix W3691.
	var choice_old   = "ancestor";  // W3691 should be "old"   -- X1/X2 content
	var choice_local = "baseline";  // W3691 should be "local" -- X1+dirt1
	var choice_new   = "other";     // W3691 should be "new"   -- X3 content

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE: " + choice_old);

	vscript_test_wc__resolve__accept(choice_old, "@/file_A.txt");

	var expect_test = new Array;
	expect_test["Modified"]             = [ "@/file_A.txt" ];	// relative to X3
	expect_test["Auto-Merged (Edited)"] = [ "@/file_A.txt" ];
	expect_test["Resolved"]             = [ "@/file_A.txt" ];
	expect_test["Choice Resolved (Contents)"] = [ "@/file_A.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        @/file_A.txt
	// Conflict:    Contents
	//              You must choose/generate the item's contents.
	// Cause:       Edit/Edit
	//              Changes to item's contents in different branches conflict.
	// Status:      Resolved (accepted ancestor)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: File's contents from the merge's common ancestor changeset
	//    baseline: File's contents from the original parent changeset
	//       other: File's contents from the merged-in changeset
	//   automerge: Merge of baseline and other using :merge from Jun 13 at 10:11
	//     working: File's current contents in the working copy (from ancestor)

	var auto_merged_content = sg.file.read("file_A.txt");
	testlib.ok( (auto_merged_content == X1_content), "Auto-Merge on file_A.txt.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE: " + choice_local);

	vscript_test_wc__resolve__accept(choice_local, "@/file_A.txt", "--overwrite");

	var expect_test = new Array;
	expect_test["Modified"]             = [ "@/file_A.txt" ];	// relative to X3
	expect_test["Auto-Merged (Edited)"] = [ "@/file_A.txt" ];
	expect_test["Resolved"]             = [ "@/file_A.txt" ];
	expect_test["Choice Resolved (Contents)"] = [ "@/file_A.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all_verbose();
	// Conflict:    Contents
	//              You must choose/generate the item's contents.
	// Cause:       Edit/Edit
	//              Changes to item's contents in different branches conflict.
	// Status:      Resolved (accepted baseline)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: File's contents from the merge's common ancestor changeset
	//    baseline: File's contents from the original parent changeset
	//       other: File's contents from the merged-in changeset
	//   automerge: Merge of baseline and other using :merge from Jun 13 at 10:19
	//     working: File's current contents in the working copy (from baseline)

	var auto_merged_content = sg.file.read("file_A.txt");
	testlib.ok( (auto_merged_content == X1_dirt1_content), "Auto-Merge on file_A.txt.");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RESOLVE: " + choice_new);

	vscript_test_wc__resolve__accept(choice_new, "@/file_A.txt", "--overwrite");

	var expect_test = new Array;
	// not modified
	expect_test["Auto-Merged (Edited)"] = [ "@/file_A.txt" ];
	expect_test["Resolved"]             = [ "@/file_A.txt" ];
	expect_test["Choice Resolved (Contents)"] = [ "@/file_A.txt" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all_verbose();
	// Item:        @/file_A.txt
	// Conflict:    Contents
	//              You must choose/generate the item's contents.
	// Cause:       Edit/Edit
	//              Changes to item's contents in different branches conflict.
	// Status:      Resolved (accepted other)
	// Choices:     Accept a value from below to change this conflict's resolution.
	//    ancestor: File's contents from the merge's common ancestor changeset
	//    baseline: File's contents from the original parent changeset
	//       other: File's contents from the merged-in changeset
	//   automerge: Merge of baseline and other using :merge from Jun 13 at 10:19
	//     working: File's current contents in the working copy (from other)

	var auto_merged_content = sg.file.read("file_A.txt");
	testlib.ok( (auto_merged_content == X3_content), "Auto-Merge on file_A.txt.");

    }
}
