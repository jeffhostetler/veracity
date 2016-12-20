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

function st_update__W1884_01_merge() 
{
    this.my_group = "st_update__W1884_01_merge";	// this variable must match the above group name.
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
	// Create the first WD and initialize the REPO.

	this.my_make_wd();
	sg.vv2.init_new_repo( { "repo" : this.repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(this.repoName);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("testing DELETE-VS-MOVE on 'a'");
	// We do this with 2 sets of items in 2 ways so that we can test RESOLVE taking
	// both the "baseline" and "other" choices each way.

        this.do_fsobj_mkdir("a1");
        this.do_fsobj_mkdir("a2");
        this.do_fsobj_mkdir("a3");
        this.do_fsobj_mkdir("a4");
        this.do_fsobj_mkdir("b1");
        this.do_fsobj_mkdir("b2");
        this.do_fsobj_mkdir("b3");
        this.do_fsobj_mkdir("b4");
        vscript_test_wc__addremove();
        this.my_commit("X0");

	vscript_test_wc__move("a1", "b1");		// now have @/b1/a1
	vscript_test_wc__move("a2", "b2");		// now have @/b2/a2
	vscript_test_wc__remove( "a3" );
	vscript_test_wc__remove( "a4" );
        this.my_commit("X1");

	// backup to X0 and create some conflicts

	this.my_update("X0");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__remove( "a1" );
	vscript_test_wc__remove( "a2" );
	vscript_test_wc__move("a3", "b3");		// now have @/b3/a3
	vscript_test_wc__move("a4", "b4");		// now have @/b4/a4
        this.my_commit("X2");

	// do regular merge (fold X1 into X2 with ancestor X0)

	this.my_merge("X1");

	// items were deleted in one side and changed in the other,
	// so the merge will re-add them.

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@/b1/a1",
					 "@/b2/a2" ];
	expect_test["Unresolved"]    = [ "@/b1/a1",
					 "@/b2/a2",
					 "@/b3/a3",
					 "@/b4/a4" ];
	expect_test["Choice Unresolved (Existence)"]    = [ "@/b1/a1",
							    "@/b2/a2",
							    "@/b3/a3",
							    "@/b4/a4" ];
	vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__resolve__list_all();
	// Existence:  @/b1/a1/
	// Existence:  @/b2/a2/
	// Existence:  @/b3/a3/
	// Existence:  @/b4/a4/

	// here "@c/" refers to X1 and "@b/" refers to X2.
	vscript_test_wc__resolve__accept("baseline", "@c/b1/a1");	// this will delete it
	vscript_test_wc__resolve__accept("other",    "@c/b2/a2");	// this will keep it (and in the X1 location)
	vscript_test_wc__resolve__accept("baseline", "@b/b3/a3");	// this will keep it (and in the X2 location)
	vscript_test_wc__resolve__accept("other",    "@b/b4/a4");	// this will delete it

	vscript_test_wc__resolve__list_all();
	// Existence:  (@c/b1/a1/)
	// Existence:  @/b2/a2/
	// Existence:  @/b3/a3/
	// Existence:  (@b/b4/a4)

	print("Post-resolve Status:");
	print(sg.to_json__pretty_print(sg.wc.status()));

	var expect_test = new Array;
	expect_test["Added (Merge)"] = [ "@c/b1/a1",
					 "@/b2/a2" ];
	expect_test["Removed"]       = [ "@c/b1/a1",
					 "@b/b4/a4" ];
	expect_test["Resolved"]      = [ "@c/b1/a1",
					 "@/b2/a2",
					 "@/b3/a3",
					 "@b/b4/a4" ];
	expect_test["Choice Resolved (Existence)"]      = [ "@c/b1/a1",
							    "@/b2/a2",
							    "@/b3/a3",
							    "@b/b4/a4" ];
	vscript_test_wc__confirm_wii(expect_test);

	// now commit the merge result as X3

	this.my_commit("X3");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);

    }
}
