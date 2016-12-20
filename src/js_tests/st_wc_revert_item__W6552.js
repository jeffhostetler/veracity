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

function st_wc_revert_item__W6552() 
{
    this.my_group = "st_wc_revert_item__W6552";	// this variable must match the above group name.
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
	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	this.my_make_wd();
	sg.vv2.init_new_repo( { "repo" : this.repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(this.repoName);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("testing DELETE-VS-MOVE on 'a'");
	// We do this with 2 sets of items so that we can test RESOLVE taking
	// both choices.

        this.do_fsobj_mkdir("a1");
        this.do_fsobj_mkdir("a2");
        this.do_fsobj_mkdir("a3");
        this.do_fsobj_mkdir("a4");
        this.do_fsobj_mkdir("b1");
        this.do_fsobj_mkdir("b2");
        this.do_fsobj_mkdir("b3");
        this.do_fsobj_mkdir("b4");
        this.do_fsobj_mkdir("c1");
        this.do_fsobj_mkdir("c2");
        vscript_test_wc__addremove();
        this.my_commit("X0");

	vscript_test_wc__move("a1", "b1");		// now have @/b1/a1
	vscript_test_wc__move("a2", "b2");		// now have @/b2/a2
	vscript_test_wc__remove( "a3" );
	vscript_test_wc__remove( "a4" );
        this.my_commit("X1");

	vscript_test_wc__move("@/b1/a1", "c1");		// now have @/c1/a1
	vscript_test_wc__move("@/b2/a2", "c2");		// now have @/c2/a2
        this.my_commit("X2");

	//////////////////////////////////////////////////////////////////
	// backup to X0 and create some dirt/conflicts

	this.my_update("X0");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__remove( "a1" );
	vscript_test_wc__remove( "a2" );
	vscript_test_wc__move("a3", "b3");		// now have @/b3/a3
	vscript_test_wc__move("a4", "b4");		// now have @/b4/a4

	// do dirty update to X1 (which should create conflicts)
	// and the details are X1-relative due to the change of basis.

	this.my_update("X1");

	var expect_test = new Array;
	expect_test["Added (Update)"]  = [ "@/b3/a3",	// WD dirt in caused them to be kept, so they
					   "@/b4/a4" ];	// appear as added-by-update relative to X1.
	expect_test["Unresolved"]     = [ "@/b1/a1",
					  "@/b2/a2",
					  "@/b3/a3",
					  "@/b4/a4" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/b1/a1",
							 "@/b2/a2",
							 "@/b3/a3",
							 "@/b4/a4" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all();
	// Existence:  @/b1/a1/
	// Existence:  @/b2/a2/
	// Existence:  @/b3/a3/
	// Existence:  @/b4/a4/

	vscript_test_wc__resolve__list_all_verbose();

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("STATUS before revert-item");
	var o = sg.exec(vv,"status", "--verbose");
	print(o.stdout);

	this.a3 = this.my_find_unique_path("@/b3/a3~*");
	this.a3gid = '@' + sg.wc.status({"src":this.a3})[0].gid;

	this.a4 = this.my_find_unique_path("@/b4/a4~*");
	this.a4gid = '@' + sg.wc.status({"src":this.a4})[0].gid;

	// The purpose of W6552 is to ensure that 'revert items'
	// properly cleans up a "added-by-update" item.  That is,
	// the revert should call the remove/unadd code that will
	// give it ADDED-by-UPDATE + REMOVED status.  What we're
	// testing here is that it also does the IMPLICIT RESOLVE.
	// See W7053.
	//
	// After the REMOVE, we need to use GID-based repo-paths
	// (because the item doesn't own a pathname any more).
	// See W7521.

	vscript_test_wc__revert_items( [ this.a3, this.a4 ] );

	vscript_test_wc__print_section_divider("STATUS after revert-item");
	var o = sg.exec(vv,"status", "--verbose");
	print(o.stdout);

	var expect_test = new Array;
	expect_test["Unresolved"]     = [ "@/b1/a1",
					  "@/b2/a2" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/b1/a1",
							 "@/b2/a2" ]
	expect_test["Added (Update)"]                = [ this.a3gid, this.a4gid ];
	expect_test["Removed"]                       = [ this.a3gid, this.a4gid ];
	expect_test["Resolved"]                      = [ this.a3gid, this.a4gid ];
	expect_test["Choice Resolved (Existence)"]   = [ this.a3gid, this.a4gid ];

	vscript_test_wc__confirm_wii(expect_test);

    }
}
