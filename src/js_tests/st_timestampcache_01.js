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

function st_timestampcache_01() 
{
    this.my_group = "st_timestampcache_01";	// this variable must match the above group name.
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

    this.nrFiles = 100;
    this.filenames = [];

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
	// CURRENTLY This test doesn't do much.  It generates a bunch
	// of files and sleeps so that we can look at the behavior of
	// the timestamp cache.  Turn on tracing in sg_timestamp_cache.c
	// *AND* in sg_wc__private.h and look at the test output.
	// See W4740.

	var content_equal = ("This is a test 1.\n"
			     +"This is a test 2.\n"
			     +"This is a test 3.\n"
			     +"This is a test 4.\n"
			     +"This is a test 5.\n"
			     +"This is a test 6.\n"
			     +"This is a test 7.\n"
			     +"This is a test 8.\n");

	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	this.my_make_wd();
	sg.vv2.init_new_repo( { "repo" : this.repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(this.repoName);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Creating initial files....");

        this.do_fsobj_mkdir("a1");
	for (var k=0; k<this.nrFiles; k++)
	{
	    this.filenames[k] = "a1/file_" + k + ".txt";
	    vscript_test_wc__write_file(this.filenames[k], content_equal);
	}
	sg.sleep_ms(4000);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Doing first ADDREMOVE and COMMIT....");

        vscript_test_wc__addremove();
        this.my_commit("X0");

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Doing first STATUS after COMMIT....");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Doing fresh checkout in new WD....");

	this.my_make_wd();
	sg.wc.checkout( { "repo" : this.repoName,
			  "attach" : "master",
			  "rev"    : this.my_names["X0"]
			} );
	sg.sleep_ms(4000);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Doing first STATUS in new WD....");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Doing second STATUS in new WD....");

	var expect_test = new Array;
	vscript_test_wc__confirm_wii(expect_test);
    }
}
