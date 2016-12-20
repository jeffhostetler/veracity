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
// A test to see how 'vv diff' displays a change in the target of
// a symlink.  This is only valid for Linux and mac.
//////////////////////////////////////////////////////////////////

function st_symlink_diff_output()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_symlink = function()
    {
	if ((sg.platform() == "LINUX") || (sg.platform() == "MAC"))
	{
	    var cset_0 = sg.wc.parents()[0];	// the initial commit

	    this.do_fsobj_mkdir_recursive("dir_a");
	    this.do_fsobj_mkdir_recursive("dir_b");

	    this.do_fsobj_create_file("file_1");
	    this.do_fsobj_create_file("dir_a/file_a");
	    this.do_fsobj_create_file("dir_b/file_b");

	    this.do_make_symlink("file_1", "link_1");
	    this.do_make_symlink("dir_a/file_a", "link_a");
	    this.do_make_symlink("dir_b/file_b", "link_b");

	    vscript_test_wc__addremove();

	    //////////////////////////////////////////////////////////////////
	    // confirm that we created what we thought we did.

	    var expect = new Array;
	    expect["Added"] = [ "@/dir_a",
				"@/dir_b",
				"@/file_1",
				"@/dir_a/file_a",
				"@/dir_b/file_b",
				"@/link_1",
				"@/link_a",
				"@/link_b" ];
	    vscript_test_wc__confirm_wii(expect);

	    // test the diff output while the items are in ADDED state.
	    // this causes the code to have to re-fetch the link target
	    // because there isn't a blob for it.

	    var r = sg.exec(vv, "diff", "link_1");
	    print(dumpObj(r, "vv diff link_1", "", 0));
	    // TODO 2011/02/02 should "vv diff" exit with 1 when there are
	    // TODO            differences (like gnu diff) (and exit with 0
	    // TODO            when there are no differences) ?
	    testlib.ok( (r.exit_status == 0), "1: Expect diff to show link added." );

	    var re1 = new RegExp("===[ \t]*Added:[ \t]*@/link_1");
	    testlib.ok( re1.test( r.stdout ), "2: Expect diff to show link added." );

	    var re2 = new RegExp("=== now -> file_1");
	    testlib.ok( re2.test( r.stdout ), "3: Expect diff to show link added." );

	    // now commit the added stuff and do a cset-vs-cset diff so that
	    // we get the other code path in the ADDED case.

	    this.do_commit("Create Stuff");
	    var cset_1 = sg.wc.parents()[0];

	    print(dumpObj(sg.exec(vv, "status", "-r", cset_0, "-r", cset_1, "--verbose"), "vv status -r 0 -r 1", "", 0));
	    print(sg.to_json__pretty_print(sg.vv2.status({"revs" : [ {"rev":cset_0}, {"rev":cset_1} ]})));
	    print(dumpObj(sg.exec(vv, "diff",   "-r", cset_0, "-r", cset_1),              "vv diff   -r 0 -r 1", "", 0));

	    var r = sg.exec(vv, "diff", "--rev="+cset_0, "--rev="+cset_1, "link_1");
	    print(dumpObj(r, "vv diff -r 0 -r 1 link_1", "", 0));
	    testlib.ok( (r.exit_status == 0),
			"Expect diff to show link added." );
	    var re1 = new RegExp("===[ \t]*Added:[ \t]*@1/link_1");
	    testlib.ok( re1.test( r.stdout ), "Expect diff to show link added." );
	    testlib.ok( (r.stdout.search("=== now -> file_1") != -1),
			"Expect diff to show link added." );

	    //////////////////////////////////////////////////////////////////
	    // now re-target one of the symlinks so we get a MODIFED status
	    // from the baseline-vs-pendingtree

	    this.do_make_symlink("dir_a/file_a", "link_1");

	    var expect = new Array;
	    expect["Modified"] = [ "@/link_1" ];
	    vscript_test_wc__confirm_wii(expect);

	    var r = sg.exec(vv, "diff", "link_1");
	    print(dumpObj(r, "vv diff link_1", "", 0));
	    testlib.ok( (r.exit_status == 0), "1: Expect diff to show link modified." );

	    var re1 = new RegExp("===[ \t]*Modified:[ \t]*@/link_1");
	    testlib.ok( re1.test( r.stdout ), "2: Expect diff to show link modified." );

	    var re2 = new RegExp("=== was -> file_1");
	    testlib.ok( re2.test( r.stdout ), "3: Expect diff to show link modified." );

	    var re3 = new RegExp("=== now -> dir_a/file_a");
	    testlib.ok( re3.test( r.stdout ), "4: Expect diff to show link modified." );

	    // now commit the change and then do a cset-vs-cset diff
	    // so we get a different code path in treediff.

	    this.do_commit("Changed Link");
	    var cset_2 = sg.wc.parents()[0];

	    var r = sg.exec(vv, "diff", "--rev="+cset_1, "--rev="+cset_2, "link_1");
	    print(dumpObj(r, "vv diff link_1", "", 0));
	    testlib.ok( (r.exit_status == 0),
			"Expect diff to show link modified." );
	    var re1 = new RegExp("===[ \t]*Modified:[ \t]*@1/link_1");
	    testlib.ok( re1.test( r.stdout ), "Expect diff to show link modified." );
	    testlib.ok( (r.stdout.search("=== was -> file_1") != -1),
			"Expect diff to show link modified." );
	    testlib.ok( (r.stdout.search("=== now -> dir_a/file_a") != -1),
			"Expect diff to show link modified." );

	    //////////////////////////////////////////////////////////////////
	    // now delete a symlink and verify we get a DELETED status in the
	    // baseline-vs-pendingtree.

	    vscript_test_wc__remove_file("link_1");

	    var expect = new Array;
	    expect["Removed"] = [ "@b/link_1" ];
	    vscript_test_wc__confirm_wii(expect);

	    var r = sg.exec(vv, "diff", "@b/link_1");
	    print(dumpObj(r, "vv diff link_1", "", 0));
	    testlib.ok( (r.exit_status == 0), "1: Expect diff to show link deleted." );
	    
	    var re1 = new RegExp("===[ \t]*Removed:[ \t\(]*@b/link_1");
	    testlib.ok( re1.test( r.stdout ), "2: Expect diff to show link deleted." );

	    var re2 = new RegExp("=== was -> dir_a/file_a");
	    testlib.ok( re2.test( r.stdout ), "3: Expect diff to show link deleted." );

	    // now commit the change and then do a cset-vs-cset diff
	    // so we get a different code path in treediff.

	    this.do_commit("Deleted Link");
	    var cset_3 = sg.wc.parents()[0];

	    var r = sg.exec(vv, "diff", "--rev="+cset_2, "--rev="+cset_3, "@0/link_1");
	    print(dumpObj(r, "vv diff link_1", "", 0));
	    testlib.ok( (r.exit_status == 0),
			"Expect diff to show link deleted." );
	    var re1 = new RegExp("===[ \t]*Removed:[ \t\(]*@0/link_1");
	    testlib.ok( re1.test( r.stdout ), "Expect diff to show link deleted." );
	    testlib.ok( (r.stdout.search("=== was -> dir_a/file_a") != -1),
			"Expect diff to show link deleted." );

	}
    }
}
