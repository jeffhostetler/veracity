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

function st_daglca_test_011()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    load("daglca_helpers.js");
    initialize_daglca_helpers(this);

    //////////////////////////////////////////////////////////////////

    this.get_node_with_hid_gt = function(parent_letter, ref_letter, prefix)
    {
	var k = 0;
	while (1)
	{
	    var letter_k = prefix + "_gt_" + k;

	    this.clean_update_to_letter(parent_letter);
	    this.create_cset(letter_k);

	    if (this.letter_map[letter_k] > this.letter_map[ref_letter])
		return letter_k;

	    k = k + 1;
	}
    }

    this.get_node_with_hid_lt = function(parent_letter, ref_letter, prefix)
    {
	var k = 0;
	while (1)
	{
	    var letter_k = prefix + "_lt_" + k;

	    this.clean_update_to_letter(parent_letter);
	    this.create_cset(letter_k);

	    if (this.letter_map[letter_k] < this.letter_map[ref_letter])
		return letter_k;

	    k = k + 1;
	}
    }
	    
    //////////////////////////////////////////////////////////////////

    this.test_daglca = function()
    {
	//            A0
	//   __________|__________
	//   |         |         |
	//  b0*       c0        d0*
	//   |         |         |
	//  b1*       c1         |
	//   |         |         |
	//   |        A1         |
	//   |  _______|_______  |
	//   |  |      |      |  |
	//   | x0*    y0     z0* |
	//   |  |      |      |  |
	//   | x1*    y1      |  |
	//   |  |      |      |  |
	//   |  |     A2      |  |
	//   |   \   /  \     |  |
	//   |    \ /    y2   |  |
	//   |    L0*      \ /   |
	//    \  /         L1*   |
	//     L2*           \  /
	//                    L3*

	this.create_cset("A0");
	this.create_cset_with_medium_hid("c0");
	this.create_cset("c1");
	this.create_cset("A1");
	this.create_cset_with_medium_hid("y0");
	this.create_cset("y1");
	this.create_cset("A2");
	this.create_cset("y2");

	//////////////////////////////////////////////////////////////////

	var b0_lt = this.get_node_with_hid_lt("A0", "c0", "b0");	// create "b0_lt_k" where we know that its hid is < hid of c0
	this.create_cset("b1_lt");

	var b0_gt = this.get_node_with_hid_gt("A0", "c0", "b0");	// create "b0_gt_k"
	this.create_cset("b1_gt");

	//////////////////////////////////////////////////////////////////

	var d0_lt = this.get_node_with_hid_lt("A0", "c0", "d0");	// create "d0_lt_k" where we know that its hid is < hid of c0

	var d0_gt = this.get_node_with_hid_gt("A0", "c0", "d0");	// create "d0_gt_k"

	//////////////////////////////////////////////////////////////////

	var x0_lt = this.get_node_with_hid_lt("A1", "y0", "x0");	// create "x0_lt_k" where we know that its hid is < hid of y0
	this.create_cset("x1_lt");

	var x0_gt = this.get_node_with_hid_gt("A1", "y0", "x0");	// create "x0_gt_k" where we know that its hid is > hid of y0
	this.create_cset("x1_gt");

	//////////////////////////////////////////////////////////////////

	var z0_lt = this.get_node_with_hid_lt("A1", "y0", "z0");	// create "z0_lt_k" where we know that its hid is < hid of y0
	var z0_gt = this.get_node_with_hid_gt("A1", "y0", "z0");	// create "z0_gt_k"

	//////////////////////////////////////////////////////////////////

	var L0 = new Array();
	this.clean_update_to_letter("A2");
	this.merge_with_letter("x1_lt", "L0_lt");	// merge "xt_lt_k" with "A2" giving "L0_lt"
	L0[0] = "L0_lt";

	this.clean_update_to_letter("A2");
	this.merge_with_letter("x1_gt", "L0_gt");	// merge "xt_gt_k" with "A2" giving "L0_gt"
	L0[1] = "L0_gt";

	//////////////////////////////////////////////////////////////////

	var L2 = new Array();
	for (k=0; k<2; k=k+1)		// create L2_lt_L0_lt, L2_lt_L0_gt, L2_gt_L0_lt, L2_gt_L0_gt
	{
	    this.clean_update_to_letter(L0[k]);
	    var name_lt = "L2_lt_" + L0[k];
	    this.merge_with_letter("b1_lt", name_lt);
	    L2[k*2] = name_lt;

	    this.clean_update_to_letter(L0[k]);
	    var name_gt = "L2_gt_" + L0[k];
	    this.merge_with_letter("b1_gt", name_gt);
	    L2[k*2+1] = name_gt;
	}
	
	//////////////////////////////////////////////////////////////////

	var L1 = new Array();
	this.clean_update_to_letter("y2");
	this.merge_with_letter(z0_lt, "L1_lt");	// merge "z0_lt_k" with "y2" giving "L1_lt"
	L1[0] = "L1_lt";

	this.clean_update_to_letter("y2");
	this.merge_with_letter(z0_gt, "L1_gt");	// merge "z0_gt_k" with "y2" giving "L1_gt"
	L1[1] = "L1_gt";

	//////////////////////////////////////////////////////////////////

	var L3 = new Array();
	for (k=0; k<2; k=k+1)		// create L3_lt_L1_lt, L3_lt_L1_gt, L3_gt_L1_lt, L3_gt_L1_gt
	{
	    this.clean_update_to_letter(L1[k]);
	    var name_lt = "L3_lt_" + L1[k];
	    this.merge_with_letter(d0_lt, name_lt);
	    L3[k*2] = name_lt;

	    this.clean_update_to_letter(L1[k]);
	    var name_gt = "L3_gt_" + L1[k];
	    this.merge_with_letter(d0_gt, name_gt);
	    L3[k*2+1] = name_gt;
	}

	//////////////////////////////////////////////////////////////////

	this.dump_letter_map();

	// these should always succeed with the same answer.
	// but currently some of these will fail because of bug S00059.

	for (var k0 = 0; k0 < 2; k0 = k0+1)
	{
	    for (var k1 = 0; k1 < 2; k1 = k1+1)
	    {
		print("");
		print("//////////////////////////////////////////////////////////////////");
		print("// Testing " + L0[k0] + " vs " + L1[k1]);
		      
		var expect_L0_L1 = new Array();
		expect_L0_L1["LCA" ] = ["A2"];
		expect_L0_L1["LEAF"] = [L0[k0], L1[k1]];
		this.compute_daglca(L0[k0], L1[k1], expect_L0_L1);
	    }
	}

	for (var k2 = 0; k2 < 4; k2 = k2 + 1)
	{
	    for (var k3 = 0; k3 < 4; k3 = k3 + 1)
	    {
		print("");
		print("//////////////////////////////////////////////////////////////////");
		print("// Testing " + L2[k2] + " vs " + L3[k3]);
		      
		var expect_L2_L3 = new Array();
		expect_L2_L3["LCA" ] = ["A2"];
		expect_L2_L3["LEAF"] = [L2[k2], L3[k3]];
		this.compute_daglca(L2[k2], L3[k3], expect_L2_L3);
	    }
	}
    }
}
