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

//////////////////////////////////////////////////////////////////

function st_device_special_files_P6060()
{
    var my_group = "st_device_special_files_P6060";	// this variable must match the above group name.

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////

    this.test_it = function()
    {
	var unique = my_group + "_" + new Date().getTime();

	var repoName = unique;
	var rootDir = pathCombine(tempDir, unique);
	var wdDirs = [];
	var csets = {};
	var nrWD = 0;

	var my_make_wd = function(obj)
	{
	    var new_wd = pathCombine(rootDir, "wd_" + nrWD);

	    vscript_test_wc__print_section_divider("Creating WD[" + nrWD + "]: " + new_wd);

	    obj.do_fsobj_mkdir_recursive( new_wd );
	    obj.do_fsobj_cd( new_wd );
	    wdDirs.push( new_wd );
	    
	    nrWD += 1;

	    return new_wd;
	}

	//////////////////////////////////////////////////////////////////
	// Create the first WD and initialize the REPO.

	my_make_wd(this);
	sg.vv2.init_new_repo( { "repo" : repoName,
				"hash" : "SHA1/160",
				"path" : "."
			      } );
	whoami_testing(repoName);

	//////////////////////////////////////////////////////////////////
	// Build a sequence of CSETs using current WD.

	this.do_fsobj_mkdir("dir_A");
	vscript_test_wc__write_file("file_1.txt", "Hello World!\n");
	vscript_test_wc__addremove();
	csets.A = vscript_test_wc__commit("A");

	this.do_fsobj_mkdir("dir_B");
	vscript_test_wc__addremove();
	csets.B = vscript_test_wc__commit("B");

	if (sg.platform() != "WINDOWS")
	{
	    var o = sg.exec("mkfifo", "fifo1");
	    testlib.ok( (o.exit_status == 0) );

	    var s = sg.wc.get_item_status_flags({"src":"fifo1"});
	    testlib.ok( (s.isFound), "found it" );
	    testlib.ok( (s.isDevice), "is device" );

	    // make sure presence of fifo does not break "revert --all".
	    // see P6060.
	    var r = sg.wc.revert_all();
	    print(sg.to_json__pretty_print(r));

	    var s = sg.wc.get_item_status_flags({"src":"fifo1"});
	    testlib.ok( (s.isFound), "found it" );
	    testlib.ok( (s.isDevice), "is device" );

	    // make sure that found fifo doesn't confuse "update".
	    var r = sg.wc.update( { "rev" : csets.A,
				    "detached" : true,
				    "verbose" : true } );
	    print(sg.to_json__pretty_print(r));

	    var s = sg.wc.get_item_status_flags({"src":"fifo1"});
	    testlib.ok( (s.isFound), "found it" );
	    testlib.ok( (s.isDevice), "is device" );
	}
	else
	{
	    // for the 2.5 release we DO NOT support WINDOWS-TYPE 
	    // FILE-SYMLINKS, DIRECTORY-SYMLINKS, nor DIRECTORY-JUNCTIONS.
	    // we treat them like other devices/special files.
	    //
	    // we require administrator privilege to be able to create
	    // one of them.
	    var bAdmin = (sg.getenv("SPRAWL_BUILDER") == "1");
	    if (bAdmin)
	    {
		// Getting "mklink" to do *ANYTHING* sane requires
		// a few tricks.
		// [1] for some reason, it doesn't exist as a real command,
		//     so we need to run CMD.exe and let it do the work.
		// [2] CMD.exe doesn't like our CWD and complains about
		//     us having a UNC path for the CWD -- and starts in
		//     C:\Windows instead -- so any relative paths are to
		//     there rather than our actual CWD.
		// [3] it doesn't like paths with forward slashes.
		// [4] it complains if the target isn't present.
		// So we make everything absolute and explicit.

		var cwd = sg.fs.getcwd().replace('/','\\','g');

		var p_f = cwd + "link_f";	var p_ft = cwd + "file_1.txt";
		var p_d = cwd + "link_d";	var p_dt = cwd + "dir_A";
		var p_j = cwd + "link_j";	var p_jt = cwd + "dir_A";

		var c_f = "mklink "    + p_f + " " + p_ft;
		var c_d = "mklink /D " + p_d + " " + p_dt;
		var c_j = "mklink /J " + p_j + " " + p_jt;

		print();
		print("CWD is: " + cwd);
		print();
		print("Exec: " + c_f);
		var o = sg.exec("CMD", "/C", c_f);
		testlib.ok( (o.exit_status == 0) );
		if (o.exit_status != 0)
		    print(sg.to_json__pretty_print(o));

		var s = sg.wc.get_item_status_flags({"src":"link_f"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		print();
		print("Exec: " + c_d);
		var o = sg.exec("CMD", "/C", c_d);
		testlib.ok( (o.exit_status == 0) );
		if (o.exit_status != 0)
		    print(sg.to_json__pretty_print(o));

		var s = sg.wc.get_item_status_flags({"src":"link_d"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		print();
		print("Exec: " + c_j);
		var o = sg.exec("cmd", "/C", c_j);
		testlib.ok( (o.exit_status == 0) );
		if (o.exit_status != 0)
		    print(sg.to_json__pretty_print(o));

		var s = sg.wc.get_item_status_flags({"src":"link_j"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		// make sure presence of link does not break "revert --all".
		// see P6060.
		var r = sg.wc.revert_all();
		print(sg.to_json__pretty_print(r));

		var s = sg.wc.get_item_status_flags({"src":"link_f"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		var s = sg.wc.get_item_status_flags({"src":"link_d"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		var s = sg.wc.get_item_status_flags({"src":"link_j"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		// make sure that found link doesn't confuse "update".
		var r = sg.wc.update( { "rev" : csets.A,
					"detached" : true,
					"verbose" : true } );
		print(sg.to_json__pretty_print(r));

		var s = sg.wc.get_item_status_flags({"src":"link_f"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		var s = sg.wc.get_item_status_flags({"src":"link_d"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		var s = sg.wc.get_item_status_flags({"src":"link_j"});
		testlib.ok( (s.isFound), "found it" );
		testlib.ok( (s.isDevice), "is device" );

		// Try to ADD the links (see also W9302)
		// and make sure we complain.

		var o = sg.exec(vv, "add", "link_f");
		testlib.ok( (o.exit_status != 0) );
		testlib.ok( (o.stderr.indexOf("Device and special files are not supported") >= 0) );

		var o = sg.exec(vv, "add", "link_d");
		testlib.ok( (o.exit_status != 0) );
		testlib.ok( (o.stderr.indexOf("Device and special files are not supported") >= 0) );

		var o = sg.exec(vv, "add", "link_j");
		testlib.ok( (o.exit_status != 0) );
		testlib.ok( (o.stderr.indexOf("Device and special files are not supported") >= 0) );

	    }
	    else
	    {
		print("Skipping mklink on Windows because not Administrator...");
	    }
	}

    }
}
