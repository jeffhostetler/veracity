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
// Tests involving merges between changesets with various patterns of path swapping.
// Two of these tests cause merge to hang.  The remainder of the tests pass.
/////////////////////////////////////////////////////////////////////////////////////

var globalThis;  // var to hold "this" pointers

function st_merge_paths_01() 
{
    this.my_group = "st_merge_paths_01";	// this variable must match the above group name.
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

    this.merge_sub = function(baseName, mergeName)
    {
	vscript_test_wc__print_section_divider("MERGE: '" + baseName + "' and '" + mergeName + "'");

	this.my_make_wd();

	// The original version of this test always did a clean update
	// back to "Merge STEM 10" as a safe point before then updating
	// to the baseName.  This was to make the tests below independent
	// of each other.  With the new version (that does not need REVERT)
	// we do each test in a new WD, so we don't have to worry about
	// such side-effects.
	//	this.my_update("Merge STEM 10");
	//	this.my_update(baseName);

	vscript_test_wc__checkout_np( { "repo"   : this.repoName,
					"attach" : "master",
					"rev"    : this.my_names[baseName]
				      } );

        this.my_merge(mergeName);
    }

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	this.my_make_wd();
	sg.vv2.init_new_repo( { "repo" : this.repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(this.repoName);

	//////////////////////////////////////////////////////////////////

        // we begin with a clean WD in a fresh REPO
        // and build a sequence of changesets to use.
	// here we create some simple (3-deep) path
	// swapping setup for merge tests.

        this.do_fsobj_mkdir("dir_merge_10");
        vscript_test_wc__addremove();
        this.my_commit("Merge STEM 10");

        this.do_fsobj_mkdir("dir_merge_10/a");
        this.do_fsobj_mkdir("dir_merge_10/a/b");
        this.do_fsobj_mkdir("dir_merge_10/a/b/c");
        vscript_test_wc__addremove();
        this.my_commit("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/b");
        vscript_test_wc__move("dir_merge_10/b/c", "dir_merge_10/b/a");
        this.my_commit("Merge HEAD 1 b/a/c");

	this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/b/c");
        this.my_commit("Merge HEAD 2 b/c/a");

        this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b/c", "dir_merge_10/a");
        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10/a/c");

        this.my_commit("Merge HEAD 3 a/c/b");

        this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b/c", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/c");

        this.my_commit("Merge HEAD 4 c/a/b");

        this.my_update("Merge STEM 10");
        this.my_update("Merge HUB 10");

        vscript_test_wc__move("dir_merge_10/a/b/c", "dir_merge_10");
        vscript_test_wc__move("dir_merge_10/a/b", "dir_merge_10/c");
        vscript_test_wc__move("dir_merge_10/a", "dir_merge_10/c/b");

        this.my_commit("Merge HEAD 5 c/b/a");

	vscript_test_wc__print_section_divider("Initial CSET Creation Completed");
	var o = sg.exec(vv, "heads");
	print("Heads are:");
	print(o.stdout);


////////////////////////////////////////////////////////////////////////////
    
	this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 2 b/c/a");
	this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 3 a/c/b");
	this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 4 c/a/b");
	this.merge_sub("Merge HEAD 1 b/a/c", "Merge HEAD 5 c/b/a");

////////////////////////////////////////////////////////////////////////////

	this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 1 b/a/c");
	this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 3 a/c/b");
	this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 4 c/a/b");
	this.merge_sub("Merge HEAD 2 b/c/a", "Merge HEAD 5 c/b/a");

////////////////////////////////////////////////////////////////////////////

	this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 1 b/a/c");

/*************************************************
 *** Prior to W2664: THIS TEST CAUSED THE PendingTree VERSION OF MERGE TO HANG (1 of 2)  ***
 *************************************************/

	print("##########################################");
	print("### test_merge_3_2 is expected to hang ...");
	print("##########################################");
	var o = sg.exec(vv, "status", "--rev", this.my_names["Merge HEAD 3 a/c/b"], "--rev", this.my_names["Merge HEAD 2 b/c/a"], "--verbose");
	print(o.stdout);
	this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 2 b/c/a");

/*************************************************
 *** Prior to W2664: THIS TEST CAUSE THE PendingTree VERSION OF MERGE TO HANG (2 of 2)  ***
 *************************************************/

	print("##########################################");
	print("### test_merge_3_4 is expected to hang ...");
	print("##########################################");
	var o = sg.exec(vv, "status", "--rev", this.my_names["Merge HEAD 3 a/c/b"], "--rev", this.my_names["Merge HEAD 2 b/c/a"], "--verbose");
	print(o.stdout);
	this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 4 c/a/b");

	this.merge_sub("Merge HEAD 3 a/c/b", "Merge HEAD 5 c/b/a");

////////////////////////////////////////////////////////////////////////////

	this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 1 b/a/c");
	this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 2 b/c/a");
	this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 3 a/c/b");
	this.merge_sub("Merge HEAD 4 c/a/b", "Merge HEAD 5 c/b/a");

////////////////////////////////////////////////////////////////////////////

	this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 1 b/a/c");
	this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 2 b/c/a");
	this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 3 a/c/b");
	this.merge_sub("Merge HEAD 5 c/b/a", "Merge HEAD 4 c/a/b");
    }
}
