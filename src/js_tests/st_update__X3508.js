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

function st_update__X3508() 
{
    this.my_group = "st_update__X3508";	// this variable must match the above group name.
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

    this.my_test = function(src, dest)
    {
	vscript_test_wc__print_section_divider("my_test: '" + src + "' and '" + dest + "'");

	this.my_make_wd();

	vscript_test_wc__checkout_np( { "repo"   : this.repoName,
					"attach" : "master",
					"rev"    : this.my_names[src]
				      } );

        this.my_update(dest);
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

	// See the text of X3508

        this.do_fsobj_mkdir("noise.txt");
        vscript_test_wc__addremove();
        this.my_commit("X0");	// trivial cset to get name

        this.do_fsobj_mkdir("a");
        this.do_fsobj_mkdir("a/b");
        vscript_test_wc__addremove();
        this.my_commit("X1");	// creatd @/a/b

	vscript_test_wc__move("a/b", ".");
	vscript_test_wc__move("a", "b");
        this.my_commit("X2");	// reordered @/b/a

	vscript_test_wc__rename("b", "a");
	this.my_commit("X3");	// now have @/a/a

	//////////////////////////////////////////////////////////////////

	vscript_test_wc__print_section_divider("Initial CSET Creation Completed");

////////////////////////////////////////////////////////////////////////////

	for (k in this.my_names)
	{
	    for (j in this.my_names)
	    {
		if (j != k)
		{
		    this.my_test(k, j);
		}
	    }
	}

    }
}
