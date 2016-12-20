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

function st_update__W1884_01__W7720_01() 
{
    this.my_group = "st_update__W1884_01__W7720_01";	// this variable must match the above group name.
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

    this.my_merge = function(name)
    {
	print("================================");
	print("Attempting MERGE with: " + name);
	print("");

	vscript_test_wc__merge_np( { "rev" : this.my_names[name], "allow_dirty" : true } );
    }

    this.my_merge__expect_error = function(name, msg)
    {
	print("================================");
	print("Attempting MERGE with: " + name);
	print("");

	vscript_test_wc__merge_np__expect_error( { "rev" : this.my_names[name], "allow_dirty" : true }, msg );
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
        this.do_fsobj_mkdir("c3");
        this.do_fsobj_mkdir("c4");
        vscript_test_wc__addremove();
        this.my_commit("X0");

	vscript_test_wc__move("a1", "b1");		// now have @/b1/a1
	vscript_test_wc__move("a2", "b2");		// now have @/b2/a2
	vscript_test_wc__remove( "a3" );
	vscript_test_wc__remove( "a4" );
        this.my_commit("X1");

	//////////////////////////////////////////////////////////////////
	// backup to X0 and create an alternate head.

	this.my_update("X0");
	vscript_test_wc__move("a1", "c1");		// now have @/c1/a1
	vscript_test_wc__move("a2", "c2");		// now have @/c2/a2
	vscript_test_wc__move("a3", "c3");		// now have @/c3/a3
	vscript_test_wc__move("a4", "c4");		// now have @/c4/a4
        this.do_fsobj_mkdir("z1");
        this.do_fsobj_mkdir("z2");
        vscript_test_wc__addremove();
        this.my_commit("Y1");

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
	expect_test["Choice Unresolved (Existence)"]     = [ "@/b1/a1",
							     "@/b2/a2",
							     "@/b3/a3",
							     "@/b4/a4" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all();
	// Existence:  @/b1/a1/
	//                  # was (non-existent)
	// Existence:  @/b2/a2/
	//                  # was (non-existent)
	// Existence:  @/b3/a3/
	//                  # was @b/b3/a3/
	// Existence:  @/b4/a4/
	//                  # was @b/b4/a4/

	//////////////////////////////////////////////////////////////////
	// W7720 asks if we can do a MERGE with outstanding resolved/unresolved issues.
	// Merge Y1 into X1+dirt.
	// For this test, we know that there ARE conflicts between X1+dirt and Y1.

	this.my_merge__expect_error("Y1", "An unresolved item would be affected");

	//////////////////////////////////////////////////////////////////
	// Resolve all of the issues, but don't commit.

	vscript_test_wc__resolve__accept("baseline", "@b/b1/a1");	// this will delete it
	vscript_test_wc__resolve__accept("other",    "@b/b2/a2");	// this will keep it (and in the X1 location)

	this.a3 = this.my_find_unique_path("@/b3/a3~*");
	if (this.a3 == undefined)
	    testlib.ok( (0), "matching a3");
	else
	    vscript_test_wc__resolve__accept("baseline", this.a3);	// this will keep it (and in the X0 location)

	this.a4 = this.my_find_unique_path("@/b4/a4~*");
	this.a4gid = '@' + sg.wc.status({"src":this.a4})[0].gid;
	if (this.a4 == undefined)
	    testlib.ok( (0), "matching a4");
	else
	    vscript_test_wc__resolve__accept("other",    this.a4);	// this will delete it

	vscript_test_wc__resolve__list_all();
	// Existence:  (non-existent)
	//                  # accepted baseline
	// Existence:  @/b2/a2/
	//                  # was (non-existent)
	//                  # accepted other
	// Existence:  @/b3/a3/
	//                  # was @b/b3/a3/
	//                  # accepted baseline
	// Existence:  (non-existent)
	//                  # was @b/b4/a4/
	//                  # accepted other

	var expect_test = new Array;
	expect_test["Added (Update)"] = [ "@/b3/a3" ];
	expect_test["Removed"]  = [ "@b/b1/a1" ];
	expect_test["Resolved"] = [ "@b/b1/a1",
				    "@/b2/a2",
				    "@/b3/a3" ];
	expect_test["Choice Resolved (Existence)"] = [ "@b/b1/a1",
						       "@/b2/a2",
						       "@/b3/a3" ];

	// See W7521.
	expect_test["Added (Update)"].push( this.a4gid );
	expect_test["Removed"].push( this.a4gid );
	expect_test["Resolved"].push( this.a4gid );
	expect_test["Choice Resolved (Existence)"].push( this.a4gid );
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	// Now try the merge again and see what it does with the resolved
	// issues when there are new merge conflicts.

	this.my_merge("Y1");

	var expect_test = new Array;
	expect_test["Added (Merge)"]   = [ "@/z1",
					   "@/z2" ];
	expect_test["Added (Update)"]  = [ "@/b3/a3",
					   "@/c4/a4" ];
	expect_test["Moved"]           = [ "@/c1/a1" ];
	expect_test["Unresolved"]     = [ "@/c1/a1",
					  "@/b2/a2",
					  "@/b3/a3",
					  "@/c4/a4" ];
	expect_test["Choice Unresolved (Existence)"]     = [ "@/c1/a1",
							     "@/c4/a4" ];
	expect_test["Choice Unresolved (Location)"]      = [ "@/b2/a2",
							     "@/b3/a3" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all();
	// Existence:  @/c1/a1/
	//                  # was (non-existent)
	// Existence:  @/c4/a4/
	//                  # was (non-existent)
	//  Location:  @/b2/a2/
	//                  # was @b/b2/a2/
	//  Location:  @/b3/a3/
	//                  # was @b/b3/a3/

	//////////////////////////////////////////////////////////////////
	// post-merge, try to resolve the conflicts from the merge.

	vscript_test_wc__resolve__accept("baseline", "@b/b1/a1");	// this will delete it
	vscript_test_wc__resolve__accept("other",    "@b/b2/a2");	// this will keep it

	this.a3 = this.my_find_unique_path("@/b3/a3~*");
	if (this.a3 == undefined)
	    testlib.ok( (0), "matching a3");
	else
	    vscript_test_wc__resolve__accept("baseline", this.a3);	// this will keep it

	this.a4 = this.my_find_unique_path("@/c4/a4~*");
	if (this.a4 == undefined)
	    testlib.ok( (0), "matching a4");
	else
	    vscript_test_wc__resolve__accept("other",    this.a4);	// this will delete it

	vscript_test_wc__resolve__list_all();
	// Existence:  (non-existent)
	//                  # accepted baseline
	// Existence:  @/c4/a4/
	//                  # was (non-existent)
	//                  # accepted other
	//  Location:  @/c2/a2/
	//                  # was @b/b2/a2/
	//                  # accepted other
	//  Location:  @/b3/a3/
	//                  # was @b/b3/a3/
	//                  # accepted baseline

	var expect_test = new Array;
	expect_test["Added (Merge)"]   = [ "@/z1",
					   "@/z2" ];
	expect_test["Added (Update)"] = [ "@/b3/a3",
					  "@/c4/a4" ];
	expect_test["Removed"]  = [ "@b/b1/a1" ];
	expect_test["Moved"]    = [ "@b/b1/a1",
				    "@/c2/a2"   ];
	expect_test["Resolved"] = [ "@b/b1/a1",
				    "@/c2/a2",
				    "@/b3/a3",
				    "@/c4/a4" ];
	expect_test["Choice Resolved (Existence)"] = [ "@b/b1/a1",
						       "@/c4/a4" ];
	expect_test["Choice Resolved (Location)"]  = [ "@/c2/a2",
						       "@/b3/a3" ];
	vscript_test_wc__confirm_wii(expect_test);

    }
}
