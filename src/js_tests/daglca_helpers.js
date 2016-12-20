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

//////////////////////////////////////////////////////////////////
// DAGLCA TESTING NOTES
// --------------------
//
// The goal of these tests are to verify that the correct ANCESTOR
// is chosen when 2 or more input csets are selected.  Normally, we
// think of the input csets as LEAVES and the ancstor as the LCA (aka
// the best ancestor to use when doing a MERGE), but nothing requires
// that they be actual leaves in the DAG; you can compare any set of
// nodes.
//
// Currently (2011/01/14) the DAGLCA code supports n-way computation,
// but the MERGE code only supports 2-way merges.  I plan to add
// n-way merges eventually.
//
// The DAGLCA tests here are only concerned with the proper ancestor
// selection and not any of the merge/conflict baggage.
//
// ===============================================================
//
// For these tests, I've created the following verbs that let you
// write a test case using node letters rather than having to deal
// with HIDs.
//
// For example: to create a series of CSets in the following structure:
//
//           A
//          / \.
//         B   C
//        / \ / \
//       D   E   F
//
// Put something like this in the test:
//
//	this.create_cset("A");
//	this.create_cset("B");
//	this.create_cset("D");
//	this.clean_update_to_letter("A");
//	this.create_cset("C");
//	this.create_cset("F");
//	this.clean_update_to_letter("B");
//      this.merge_with_letter("C", "E");   // merge B with C creating E
//
// Each create_cset() command creates a new unique cset (relative to
// the current WD/parent) and than TAGs it with the letter so that
// subsequent verbs can reference it by tag.
//
// Each clean_update_to_letter() command does an UPDATE to the named
// letter (and cleans up any trash).
//
// Each merge_with_letter() uses the current WD/parent and the first
// arg to create a unique merge (which is TAGGED with the second arg).
//
// ===============================================================
//
// Wthe the graph constructed, you can run various DAGLCA requests
// to test it.  Build an "expect" array with LEAF, LCA, and (sometimes)
// SPCA cells that each contain a list of the expected values.
//
// The compute_daglca_n() will run the computation and verify that
// the output EXACTLY matches the EXPECTED values.
//
//	var expect = new Array();
//	expect["LEAF"] = ["B", "C"];
//	expect["LCA" ] = ["A"];
//	this.compute_daglca_n(expect);
//
//	var expect = new Array();
//	expect["LEAF"] = ["D", "F"];
//	expect["LCA" ] = ["A"];
//	this.compute_daglca_n(expect);
//
//	var expect = new Array();
//	expect["LEAF"] = ["D", "E", "F"];
// 	expect["SPCA"] = ["B", "C"];
//	expect["LCA" ] = ["A"];
//	this.compute_daglca_n(expect);
//
// ===============================================================
//
// The LCA is the closest UNIQUE node to *ALL* of the named
// leaves/inputs that is an ancestor of all of them.
//
// An SPCA is a "somewhat significant" ancestor of 2 or more inputs.
// An SPCA might be a "partial" ancestor (an ancestor or only some of
// the inputs).  Or there may be criss-cross type links in the graph
// such that the ancestor has a peer.  In both cases, we mark the
// node as significant and keep searching for an older node that
// is unique.
//
// The above example is taken from st_daglca_tests_014.js
// For a criss-cross example, see st_daglca_tests_010.js
//
//////////////////////////////////////////////////////////////////

function initialize_daglca_helpers(obj)
{
    obj.letter_map = new Array();
    obj.current_letter = "_unset_";

    //////////////////////////////////////////////////////////////////

    obj.label_initial_cset = function(letter)
    {
	// this must be called first (before any user csets are created)
	// if you want to reference the initial cset.
	// (because i'm too lazy here to do a history and extract the
	// oldest cset.)

	obj.letter_map[letter] = sg.wc.parents()[0];
	sg.exec(vv, "tag", "add", letter);
	print("Labelling initial cset: " + letter + " CSetId: " + obj.letter_map[letter]);

	obj.current_letter = letter;
    }

    obj.create_cset = function(letter)
    {
	print("Creating node: " + letter);

	obj.do_fsobj_create_file( letter );
	var o1 = sg.exec(vv, "add", letter);
	var o2 = sg.exec(vv, "commit", "-m", "add " + letter);
	var o3 = sg.exec(vv, "tag", "add", letter);

	obj.letter_map[letter] = sg.wc.parents()[0];
	print("Node: " + letter + " CSetId: " + obj.letter_map[letter]);

	obj.current_letter = letter;
    }

    obj.create_cset_with_medium_hid = function(letter)
    {
	// create a named node but with a HID in the middle of the range.
	// this helps when we are trying to get an ordering of HIDs for
	// certain tests.

	print("Creating node: " + letter);

	var letter_start = obj.current_letter;
	var attempt = 1;
	while (1)
	{
	    var filename = letter + "_attempt_" + attempt;

	    obj.do_fsobj_create_file( filename );
	    var o1 = sg.exec(vv, "add", filename);
	    var o2 = sg.exec(vv, "commit", "-m", "add " + filename);

	    var hid = sg.wc.parents()[0];
	    if ((hid[0] >= "4") && (hid[0] <= "c"))
	    {
		var o3 = sg.exec(vv, "tag", "add", letter);

		obj.letter_map[letter] = sg.wc.parents()[0];
		print("MediumNode: " + letter + " CSetId: " + obj.letter_map[letter]);

		obj.current_letter = letter;
		return;
	    }

	    attempt = attempt + 1;
	    obj.clean_update_to_letter(letter_start);
	}
    }

    obj.clean_update_to_letter = function(letter)
    {
	obj.force_clean_WD();

	print("Updating to node: " + letter + " (from " + obj.current_letter + ")" );
	
	vscript_test_wc__update( obj.letter_map[letter] );
	sg.exec(vv, "branch", "attach", "master");

	obj.current_letter = letter;
	print("Update complete to node: " + letter);
    }

    obj.merge_with_letter = function(letter_merge_with, letter_result)
    {
	var msg = "Merging node: " + letter_merge_with + " into current node: " + obj.current_letter + " giving: " + letter_result;
	print(msg);

	var o1 = sg.exec(vv, "merge", "--rev", obj.letter_map[letter_merge_with]);
	var o2 = sg.exec(vv, "commit", "-m", msg);
	var o3 = sg.exec(vv, "tag", "add", letter_result);

	obj.letter_map[letter_result] = sg.wc.parents()[0];
	print("Node: " + letter_result + " CSetId: " + obj.letter_map[letter_result]);

	obj.current_letter = letter_result;
    }

    obj.merge_unique_with_letter = function(letter_merge_with, letter_result)
    {
	var msg = "Merging(unique) node: " + letter_merge_with + " into current node: " + obj.current_letter + " giving: " + letter_result;
	print(msg);

	var o1 = sg.exec(vv, "merge", "--rev", obj.letter_map[letter_merge_with]);
	print(sg.exec(vv, "status").stdout);
	obj.do_fsobj_create_file(letter_result);	// add some dirt after the merge
	var oa = sg.exec(vv, "add", letter_result);	// so that we get a unique result cset
	var o2 = sg.exec(vv, "commit", "-m", msg);
	var o3 = sg.exec(vv, "tag", "add", letter_result);

	obj.letter_map[letter_result] = sg.wc.parents()[0];
	print("Node: " + letter_result + " CSetId: " + obj.letter_map[letter_result]);

	obj.current_letter = letter_result;
    }

    obj.merge_unique_with_letter__and_edit = function(letter_merge_with, letter_result, file_to_edit)
    {
	var msg = "Merging(unique) node: " + letter_merge_with + " into current node: " + obj.current_letter + " giving: " + letter_result + " with edit on " + file_to_edit;
	print(msg);

	var o1 = sg.exec(vv, "merge", "--rev", obj.letter_map[letter_merge_with]);
	print(sg.exec(vv, "status").stdout);
	obj.do_fsobj_create_file(letter_result);	// add some dirt after the merge
	var oa = sg.exec(vv, "add", letter_result);	// so that we get a unique result cset

	this.do_fsobj_append_file(file_to_edit, "Edited in " + letter_result + "\n");

	var o2 = sg.exec(vv, "commit", "-m", msg);
	var o3 = sg.exec(vv, "tag", "add", letter_result);

	obj.letter_map[letter_result] = sg.wc.parents()[0];
	print("Node: " + letter_result + " CSetId: " + obj.letter_map[letter_result]);

	obj.current_letter = letter_result;
    }

    obj.merge_unique_with_letter__and_rename = function(letter_merge_with, letter_result, file_to_edit, new_name)
    {
	var msg = "Merging(unique) node: " + letter_merge_with + " into current node: " + obj.current_letter + " giving: " + letter_result + " with rename of " + file_to_edit + " to " + new_name;
	print(msg);

	var o1 = sg.exec(vv, "merge", "--rev", obj.letter_map[letter_merge_with]);
	print(sg.exec(vv, "status").stdout);
	obj.do_fsobj_create_file(letter_result);	// add some dirt after the merge
	var oa = sg.exec(vv, "add", letter_result);	// so that we get a unique result cset

	vscript_test_wc__rename(file_to_edit, new_name);

	var o2 = sg.exec(vv, "commit", "-m", msg);
	var o3 = sg.exec(vv, "tag", "add", letter_result);

	obj.letter_map[letter_result] = sg.wc.parents()[0];
	print("Node: " + letter_result + " CSetId: " + obj.letter_map[letter_result]);

	obj.current_letter = letter_result;
    }

    obj.compute_daglca_n = function(expected)
    {
	// use expected[LEAF][...] to drive the exec and not require
	// the args to be repeated.

	var cl = "var out = sg.exec(vv, \"dump_lca\", \"--list\" ";
	for (var e_index in expected["LEAF"])
	{
	    cl = cl + ", \"--tag\", \"" + expected["LEAF"][e_index] + "\"";
	}
	cl = cl + ");";

	print("Evaluating: " + cl);
	eval(cl);

	return obj.verify_daglca_result(expected, out);
    }

    obj.compute_daglca = function(letter_1, letter_2, expected)
    {
	print("Computing DAGLCA for: " + letter_1 + " and " + letter_2);
	var out = sg.exec(vv, "dump_lca", "--list", "--tag", letter_1, "--tag", letter_2);
	return obj.verify_daglca_result(expected, out);
    }

    obj.verify_daglca_result = function(expected, out)
    {
	var success = true;

	if (out.exit_status != 0)
	{
	    testlib.ok( (0), "Exit status of 'vv dump_lca' should be zero.");
	    print("//////////////////////////////////////////////////////////////////");
	    print("out.stdout:");
	    print(out.stdout);
	    print("//////////////////////////////////////////////////////////////////");
	    print("out.stderr:");
	    print(out.stderr);
	    print("//////////////////////////////////////////////////////////////////");

	    success = false;
	}
	else
	{
	    var rows = out.stdout.replace(/\r/g,"").split("\n");
	    var nrRows = rows.length;
	    if (rows[nrRows-1].length == 0)		// ignore trailing blank line 
		nrRows = nrRows-1;

	    var claimed = new Array(nrRows);
	    for (var k = 0; k < nrRows; k++)
	    {
		print("Row[" + k + "]: " + rows[k]);
		claimed[k] = false;
	    }

	    for (var e_section in expected)
	    {
		for (var e_index in expected[e_section])
		{
		    var letter = expected[e_section][e_index];
		    var m = obj.letter_map[ letter ] + " " + e_section;
		    var found = false;

		    for (var k = 0; k < nrRows; k++)
		    {
			if (m == rows[k])
			{
			    claimed[k] = true;
			    found = true;
			}
		    }

		    testlib.ok( found, "Expected[ " + e_section + " ][ " + letter + " ]." );
		    if (!found)
			success = false;
		}
	    }

	    for (var k = 0; k < nrRows; k++)
	    {
		testlib.ok( claimed[k], "Matched: " + rows[k] );
		if (!claimed[k])
		    success = false;
	    }
	}

	return success;
    }

    obj.dump_letter_map = function()
    {
	// print a table of the "letters => hid" mappings.

	print("Letter Map:");
	for (var e_letter in obj.letter_map)
	{
	    print( e_letter + "\t" + obj.letter_map[e_letter] );
	}
    }
}
