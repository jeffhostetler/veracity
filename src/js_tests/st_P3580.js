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

function st_P3580()
{
    this.my_group = "st_P3580";		// this variable must match the above group name.
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

    this.nrFiles = 4;
    this.filenames = [];
    this.FI_filenames = [];

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

    this.my_stat = function(msg, delay)
    {
	vscript_test_wc__print_section_divider( msg );

	// optional delay so that the clock has ticked
	// enough for us to create new entries in the
	// timestamp cache when we see files.

	if (delay > 0)
	    sg.sleep_ms(delay);

	// do a status/scan for the side-effect of
	// causing the timestamp cache to be updated.

	var status = sg.wc.status();

	// report how big the timestamp cache is.
	var tsc = "./.sgdrawer/timestampcache.rbtreedb";
	var len = sg.fs.length(tsc);
	print("TSC: " + len);

	return len;
    }

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
	var content_equal = ("\n");
	var content_FI    = ("1234567890\n");

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

	vscript_test_wc__write_file(".sgignores", 
				    ("*.FI\n"
				     +"*.DI\n"));

        this.do_fsobj_mkdir("a1");
	for (var k=0; k<this.nrFiles; k++)
	{
	    this.filenames[k] = "a1/file_" + k + ".txt";
	    vscript_test_wc__write_file(this.filenames[k], content_equal);
	}
        vscript_test_wc__addremove();
        this.my_commit("X0");

	this.my_stat("immediately after commit", 0);
	this.my_stat("4 seconds after commit", 4000);
	var len1 = this.my_stat("8 seconds after commit", 4000);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Creating some ignored files....");

	var FI_buf = ".";
	for (var k=0; k<this.nrFiles; k++)
	{
	    FI_buf = FI_buf + ".";
	    this.FI_filenames[k] = "a1/FI__" + k + ".FI";
	    vscript_test_wc__write_file(this.FI_filenames[k], FI_buf + content_FI);
	}

	this.my_stat("immediately after create FI", 0);
	this.my_stat("4 seconds after create FI", 4000);
	var len2 = this.my_stat("8 seconds after create FI", 4000);

	testlib.ok( (len2 == len1), "IGNORED items should not get added to the TSC.");
    }
}
