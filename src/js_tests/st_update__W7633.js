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

function st_update__W7633() 
{
    this.my_group = "st_update__W7633";	// this variable must match the above group name.
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
	// W8402 asks if we can do a DIRTY UPDATE with UNRESOLVED ISSUES.
	//
	// If the SECOND UPDATE will need to make *any* change to an ITEM
	// with an OUTSTANDING UNRESOLVED ISSUE, we should complain.
	// (This is more restictive than saying we just throw when we need
	// to create an issue when there is one already.)
	//
	// Note that because of the random GID thing, we probably can't
	// predict which of the unresolved items will appear in the error
	// message.

	this.my_update__expect_error("X2", "An unresolved item would be affected");

	// we still have baseline of X1.  resolve the things that changed
	// between X1 and X2.  Try to carry forward the reset.

	vscript_test_wc__resolve__accept("baseline", "@b/b1/a1");	// this will delete it
	vscript_test_wc__resolve__accept("other",    "@b/b2/a2");	// this will keep it (and in the X1 location)

	var expect_test = new Array;
	expect_test["Removed"]  = [ "@b/b1/a1" ];
	expect_test["Resolved"] = [ "@b/b1/a1",
				    "@/b2/a2"  ];
	expect_test["Choice Resolved (Existence)"] = [ "@b/b1/a1",
						       "@/b2/a2"  ];

	expect_test["Added (Update)"]  = [ "@/b3/a3",	// WD dirt in caused them to be kept, so they
					   "@/b4/a4" ];	// appear as added-by-update relative to X1.
	expect_test["Unresolved"]     = [ "@/b3/a3",
					  "@/b4/a4" ];
	expect_test["Choice Unresolved (Existence)"] = [ "@/b3/a3",
							 "@/b4/a4" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all();
	// Existence:  (non-existent)
	//                  # accepted baseline
	// Existence:  @/b2/a2/
	//                  # was (non-existent)
	//                  # accepted other
	// Existence:  @/b3/a3/
	//                  # was @b/b3/a3/
	// Existence:  @/b4/a4/
	//                  # was @b/b4/a4/

	this.my_update("X2");

	// after the first update to X1 and resolve, we have a pended delete of a1.
	// since it was moved again in X2, we must re-decide it's existence.
	//
	// as for a2, we chose what X1 did to it rather than the dirty delete,
	// so we can silently get the change that X2 made to it.
	// note how a2 does not appear in the resolved list -- per W2065.

	var expect_test = new Array;
	expect_test["Added (Update)"] = [ "@/b3/a3",	// WD dirt in caused them to be kept, so they NOW
					  "@/b4/a4" ];	// appear as added-by-update relative to X2.
	expect_test["Unresolved"]     = [ "@/c1/a1",
					  "@/b3/a3",
					  "@/b4/a4" ];
	expect_test["Choice Unresolved (Existence)"]     = [ "@/c1/a1",
							     "@/b3/a3",
							     "@/b4/a4" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all();
	// Existence:  @/c1/a1/
	//                  # was (non-existent)
	// Existence:  @/b3/a3/
	//                  # was @b/b3/a3/
	// Existence:  @/b4/a4/
	//                  # was @b/b4/a4/

	//////////////////////////////////////////////////////////////////
	// now try to resolve the stuff we carried forward.

	vscript_test_wc__resolve__accept("baseline", "@b/c1/a1");	// this will delete it
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

	var expect_test = new Array;
	expect_test["Added (Update)"] = [ "@/b3/a3" ];
	expect_test["Removed"]  = [ "@b/c1/a1" ];
	expect_test["Resolved"] = [ "@b/c1/a1",
				    "@/b3/a3" ];
	expect_test["Choice Resolved (Existence)"] = [ "@b/c1/a1",
						       "@/b3/a3" ];

	// See W7521.
	expect_test["Added (Update)"].push( this.a4gid );
	expect_test["Removed"].push( this.a4gid );
	expect_test["Resolved"].push( this.a4gid );
	expect_test["Choice Resolved (Existence)"].push( this.a4gid );
	vscript_test_wc__confirm_wii(expect_test);

    }
}
