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

function st_daglca_test_009()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    load("daglca_helpers.js");
    initialize_daglca_helpers(this);

    //////////////////////////////////////////////////////////////////

    this.test_daglca = function()
    {
	//            A0
	//        _____|___________________
	//        |      |      |         |
	//       x0     y0     z0_0 .... z0_n
	//        |      |      |         |
	//       x1     y1      |         |
	//        |      |      |         |
	//        |     A1      |         |
	//         \   /  \     |         |
	//          \ /    y2   |         |
	//          L0       \ /       \ /
	//                    L1_0 .... L1_n


	this.create_cset("A0");
	this.create_cset_with_medium_hid("y0");
	this.create_cset("y1");
	this.create_cset("A1");
	this.create_cset("y2");

	// create a series of x0's so that we get one with a HID < the HID of y0
	// and one with a HID > y0.  Then create the correspoding L0's.

	var x0_lt = "";
	var x0_gt = "";
	var L0_lt = "";
	var L0_gt = "";
	var k = 0;
	while ((x0_lt == "") || (x0_gt == ""))
	{
	    var x0_k = "x0_" + k;
	    var x1_k = "x1_" + k;
	    var L0_k = "L0_" + k;

	    this.clean_update_to_letter("A0");
	    this.create_cset(x0_k);

	    if (this.letter_map[x0_k] < this.letter_map["y0"])
	    {
		if (x0_lt == "")
		{
		    print("x0_lt test using: " + x0_k + " and " + L0_k);
		    this.create_cset(x1_k);
		    this.merge_with_letter("A1", L0_k);

		    x0_lt = x0_k;
		    L0_lt = L0_k;
		}
	    }
	    else
	    {
		if (x0_gt == "")
		{
		    print("x0_gt test using: " + x0_k + " and " + L0_k);
		    this.create_cset(x1_k);
		    this.merge_with_letter("A1", L0_k);

		    x0_gt = x0_k;
		    L0_gt = L0_k;
		}
	    }

	    k = k + 1;
	}

	// create a series of z0's so that we get one with a HID < the HID of y0
	// and one with a HID > y0.  Then create a corresponding L1's.

	var z0_lt = "";
	var z0_gt = "";
	var L1_lt = "";
	var L1_gt = "";
	var k = 0;
	while ((z0_lt == "") || (z0_gt == ""))
	{
	    var z0_k = "z0_" + k;
	    var L1_k = "L1_" + k;

	    this.clean_update_to_letter("A0");
	    this.create_cset(z0_k);

	    if (this.letter_map[z0_k] < this.letter_map["y0"])
	    {
		if (z0_lt == "")
		{
		    print("z0_lt test using: " + z0_k + " and " + L1_k);
		    this.merge_with_letter("y2", L1_k);
		    z0_lt = z0_k;
		    L1_lt = L1_k;
		}
	    }
	    else
	    {
		if (z0_gt == "")
		{
		    print("z0_gt test using: " + z0_k + " and " + L1_k);
		    this.merge_with_letter("y2", L1_k);
		    z0_gt = z0_k;
		    L1_gt = L1_k;
		}
	    }

	    k = k + 1;
	}

	this.dump_letter_map();

	// these should always succeed with the same answer.
	// but currently some of these will fail because of bug S00059.

	var expect_gt_gt = new Array();
	expect_gt_gt["LCA" ] = ["A1"];
	expect_gt_gt["LEAF"] = [L0_gt, L1_gt];
	this.compute_daglca(L0_gt, L1_gt, expect_gt_gt);

	var expect_gt_lt = new Array();
	expect_gt_lt["LCA" ] = ["A1"];
	expect_gt_lt["LEAF"] = [L0_gt, L1_lt];
	this.compute_daglca(L0_gt, L1_lt, expect_gt_lt);

	var expect_lt_gt = new Array();
	expect_lt_gt["LCA" ] = ["A1"];
	expect_lt_gt["LEAF"] = [L0_lt, L1_gt];
	this.compute_daglca(L0_lt, L1_gt, expect_lt_gt);

	var expect_lt_lt = new Array();
	expect_lt_lt["LCA" ] = ["A1"];
	expect_lt_lt["LEAF"] = [L0_lt, L1_lt];
	this.compute_daglca(L0_lt, L1_lt, expect_lt_lt);

    }
}
