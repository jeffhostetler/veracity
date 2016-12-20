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
// Some tests to verify argv/depth is respected in historical/vv2
// diffs and status.  W3440, W9284.
//////////////////////////////////////////////////////////////////

function st_vv2_diff_status_argv__W3440_W9284() 
{
    // Helper functions are in update_helpers.js
    // There is a reference list at the top of the file.
    // After loading, you must call initialize_update_helpers(this)
    // before you can use the helpers.
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    // this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later
        this.generate_test_changesets();
    }

    this.compare_status_items1 = function( s0, s1 )
    {
	if (s0.status.flags != s1.status.flags)
	    return false;
	if (s0.gid != s1.gid)
	    return false;
	if (s0.path != s1.path)
	    return false;
	if ((s0.A == undefined) != (s1.A == undefined))
	    return false;
	if ((s0.B == undefined) != (s1.B == undefined))
	    return false;

	if (s0.A != undefined)
	{
	    if (s0.A.path != s1.A.path)
		return false;
	    if (s0.A.name != s1.A.name)
		return false;
	    if (s0.A.gid_parent != s1.A.gid_parent)
		return false;
	}
	if (s0.B != undefined)
	{
	    if (s0.B.path != s1.B.path)
		return false;
	    if (s0.B.name != s1.B.name)
		return false;
	    if (s0.B.gid_parent != s1.B.gid_parent)
		return false;
	}

	return true;
    }
    this.compare_status_items = function( s0, s1, msg )
    {
	var eq = this.compare_status_items1(s0, s1);
	testlib.ok( (eq), "compare_status_items: " + msg);
	if (!eq)
	{
	    print("s0 is:");
	    print(sg.to_json__pretty_print(s0));
	    print("s1 is:");
	    print(sg.to_json__pretty_print(s1));
	}
	return eq;
    }

    this.get_r = function( name )
    {
	var ndx = this.name2ndx[name];
	var r   = this.csets[ndx];
	return r;
    }

    this.my_vv2_full_status = function( name0, name1 )
    {
	var r0 = this.get_r(name0);
	var r1 = this.get_r(name1);

	print("Full status of ['" + name0 + "' ==> '" + name1 + "']:");
	print("               ['" + r0    + "' ==> '" + r1    + "']:");
	var full_status = sg.vv2.status( { "revs" : [ {"rev" : r0}, {"rev" : r1} ] } );
	print("Full status contains " + full_status.length + " items.");
	print(sg.to_json__pretty_print(full_status));

	return full_status;
    }

    this.my_vv2_filtered_status = function( name0, name1, src, depth )
    {
	var r0 = this.get_r(name0);
	var r1 = this.get_r(name1);

	print("Filtered status of ['" + name0 + "' ==> '" + name1 + "']:");
	print("                   ['" + r0    + "' ==> '" + r1    + "']:");
	var filtered_status = sg.vv2.status( { "revs" : [ {"rev" : r0}, {"rev" : r1} ],
					       "src"  : src,	// can be string or array of string
					       "depth" : depth } );
	print("Filtered status contains " + filtered_status.length + " items.");
	print(sg.to_json__pretty_print(filtered_status));

	return filtered_status;
    }

    this.try_pair = function(name0, name1)
    {
	var full_status = this.my_vv2_full_status(name0, name1);
	for (k in full_status)
	{
	    var gid_k = full_status[k].gid;
	    print("Looking at ["+k+"]: " + gid_k);

	    var g_status = this.my_vv2_filtered_status(name0, name1, "@"+gid_k, 0);
	    testlib.ok( (g_status.length == 1), "Found 1 item when looking up by GID (non-recursive)." );
	    this.compare_status_items(g_status[0], full_status[k], "Do contents match when looking up by GID.");


	    var p_k = full_status[k].path;
	    testlib.ok( ((p_k[0]=='@') && ((p_k[1]=='0') || (p_k[1]=='1')) && (p_k[2]=='/')), "Does path have prefix: "+p_k);
	    var p_status = this.my_vv2_filtered_status(name0, name1, p_k, 0);
	    testlib.ok( (p_status.length == 1), "Found 1 item when looking up by repopath (non-recursive)." );
	    testlib.ok( (p_status[0].gid == gid_k), "Did repopath lookup find same status item.");


	    if (full_status[k].A != undefined)
	    {
		var p0_k = full_status[k].A.path;
		testlib.ok( ((p0_k[0]=='@') && (p0_k[1]=='0') && (p0_k[2]=='/')), "Does path have prefix: "+p0_k);
		var p0_status = this.my_vv2_filtered_status(name0, name1, p0_k, 0);
		testlib.ok( (p0_status.length == 1), "Found 1 item when looking up by repopath[0] (non-recursive)." );
		testlib.ok( (p0_status[0].gid == gid_k), "Did repopath lookup find same status item.");
	    }		
	    if (full_status[k].B != undefined)
	    {
		var p1_k = full_status[k].B.path;
		testlib.ok( ((p1_k[0]=='@') && (p1_k[1]=='1') && (p1_k[2]=='/')), "Does path have prefix: "+p1_k);
		var p1_status = this.my_vv2_filtered_status(name0, name1, p1_k, 0);
		testlib.ok( (p1_status.length == 1), "Found 1 item when looking up by repopath[0] (non-recursive)." );
		testlib.ok( (p1_status[0].gid == gid_k), "Did repopath lookup find same status item.");
	    }		

	}
    }

    this.test_overall_nonrecursive = function()
    {
	this.try_pair( "Simple ADDs 0", "Swap directory RENAMEs 9" );
	this.try_pair( "Swap directory RENAMEs 9", "Simple ADDs 0" );
	this.try_pair( "Simple ADDs 4", "Simple ADDs 5" );

    }

    this.test_recursive__6 = function()
    {
	var name0 = "Simple ADDs 6";
	var name1 = "Simple file MODIFY 6.3";

	var full_status = this.my_vv2_full_status(name0, name1);
	var expect_test = new Array;
	expect_test["Modified"] = [ "@1/dir_6/dir_66/file_666",
				    "@1/dir_6/file_66",
				    "@1/file_6" ];
	vscript_test_wc__confirm(expect_test, full_status);
	
	var d0_status = this.my_vv2_filtered_status(name0, name1, "@1/dir_6", 0);
	var expect_test = new Array;
	vscript_test_wc__confirm(expect_test, full_status);


	var d1_status = this.my_vv2_filtered_status(name0, name1, "@1/dir_6", 1);
	var expect_test = new Array;
	expect_test["Modified"] = [ "@1/dir_6/file_66" ];
	vscript_test_wc__confirm(expect_test, full_status);


	var d2_status = this.my_vv2_filtered_status(name0, name1, "@1/dir_6", 2);
	var expect_test = new Array;
	expect_test["Modified"] = [ "@1/dir_6/dir_66/file_666",
				    "@1/dir_6/file_66" ];
	vscript_test_wc__confirm(expect_test, full_status);

    }

}
