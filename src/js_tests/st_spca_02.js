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
// Some DAGLCA tests.
//////////////////////////////////////////////////////////////////

function st_spca_01()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    load("daglca_helpers.js");
    initialize_daglca_helpers(this);

    //////////////////////////////////////////////////////////////////

    this.test_daglca = function()
    {
	// generate an arbitrarily long lattice of criss-crosses
	// to try to blow up the daglca code.
	//
	// for any given pair of leaves (n[j][0],n[j][1]) the LCA
	// is A and there are ((j-1)*2) SPCAs, so the daglca code
	// should require (j*2 + 1) bits in the bitvector.
	//
	//           A
	//          / \.
	//   n[0][0]   n[0][1]
	//         |\ /|
	//         |/ \|
	//   n[1][0]   n[1][1]
	//         |\ /|
	//         |/ \|
	//   n[2][0]   n[2][1]
	//         |\ /|
	//           .
	//           .
	//           .
	//         |/ \|
	//   n[k][0]   n[k][1]
	//
	// The helper functions tag each created changeset with
	// the name/letter we give it.  Since tags no longer
	// support [] characters, names like n[x][y] are
	// sanitized to n-x-y.

	this.create_cset("A");

	var N = new Array;
	N[0] = new Array;
	var p0 = "n-0-0";
	this.create_cset(p0);
	this.clean_update_to_letter("A");
	var p1 = "n-0-1";
	this.create_cset(p1);

	var expect = new Array();
	expect["LCA" ] = ["A"];
	expect["SPCA"] = new Array();
	expect["LEAF"] = [p0, p1];
	this.compute_daglca_n(expect);

	// pick an arbitrary file to tinker with.
	var f = "n-5-0";	// the surplus file created in that cset.
	var s0 = "n-6-0";	// the first tier of SPCAs that will see f.
	var s1 = "n-6-1";
	var s2 = "n-7-0";	// the next tier of SPCAs that will see merge of s0 and s1.
	var s3 = "n-7-1";

	var kLimit = 10;
	for (var k=1; k<kLimit; k++)
	{
	    // enter loop with p0 === n[k-1][0] and p1 === n[k-1][1]
	    // and WD set to p1.

	    var m0 = "n-" + k + "-0";
	    var m1 = "n-" + k + "-1";

	    print("");
	    print("//////////////////////////////////////////////////////////////////");
	    print("Beginning level [" + k + "]:");

	    if (m0 == s0)
		this.merge_unique_with_letter__and_edit(p0, m0, f);	// merge(p1,p0) ==> m0
	    else
		this.merge_unique_with_letter(p0, m0);			// merge(p1,p0) ==> m0
	    this.clean_update_to_letter(p0);
	    this.merge_unique_with_letter(p1, m1);			// merge(p0,p1) ==> m1

	    // leave expect["LCA"] set to A
	    expect["SPCA"].push( p0 );		// accumulate n[0..k-1][0..1] in SPCA
	    expect["SPCA"].push( p1 );		// to reflect criss-cross ladder.
	    expect["LEAF"] = [m0, m1];		// reset leaves to new heads.
	    this.compute_daglca_n(expect);

	    p0 = m0;
	    p1 = m1;
	}

	this.dump_letter_map();

	// let both p0 and p1 edit a file that was
	// created somewhere within the lattice and
	// not present in the LCA and create p0a and p1a.
	//
	// then try to merge them.
	//
	// this should cause a problem because the auto-merge
	// code wants to find the ancestor version of the file
	// in the LCA and it didn't exist in the LCA.

	// we are currently looking at p1.

	var p1a = p1 + "_a";
	this.do_fsobj_append_file(f, "Edited in " + p1a);
	sg.exec(vv, "commit", "-m", "Edited in " + p1a);
	sg.exec(vv, "tag", "add", p1a);

	this.clean_update_to_letter(p0);

	var p0a = p0 + "_a";
	this.do_fsobj_append_file(f, "Edited in " + p0a);
	sg.exec(vv, "commit", "-m", "Edited in " + p0a);
	sg.exec(vv, "tag", "add", p0a);

	var result_merge = sg.exec(vv, "merge", "--tag", p1a);
	print(dumpObj(result_merge,"merge","",0));
	var result_status = sg.exec(vv, "status", "--verbose");
	print(dumpObj(result_status,"status","",0));

	var expect_test = new Array;
	expect_test["Added (Merge)"]    = [ "@/" + p1 ];
	expect_test["Modified"]         = [ "@/" + f  ];
	expect_test["Auto-Merged"]      = [ "@/" + f  ];
	expect_test["Unresolved"]       = [ "@/" + f  ];
	expect_test["Choice Unresolved (Contents)"] = [ "@/" + f  ];
	vscript_test_wc__confirm_wii(expect_test);

	// since the file first appears in f, it will also
	// appear as an add in both of the immediate descendants
	// in the ladder ( s0 and s1 ).
	// 
	// since s0 changed the file, we'll get another set of
	// peers in s2 and s3 when s0 and s1 are merged.  both
	// s2 and s3 should get the same result.
	//
	// so both s2 and s3 are equivalent and an equally good
	// choice for the ancestor for this file.
	//
	// so we expect the 'issue.A' field to be bound to one of
	// these parents rather than the LCA (where it isn't present).
	// the choice is random (based upon HIDs).
	var issue = sg.wc.get_item_issues({"src": "@/" + f});
	testlib.ok( ((issue.input.A.cset_hid == this.letter_map[s2]) 
		     || (issue.input.A.cset_hid == this.letter_map[s3])),
		    "Expect issue.A to be either s2 or s3.");
    }
}
