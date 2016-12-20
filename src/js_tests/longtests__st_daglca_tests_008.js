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

function st_daglca_test_008()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    load("daglca_helpers.js");
    initialize_daglca_helpers(this);

    //////////////////////////////////////////////////////////////////

    this.test_daglca = function()
    {
	//            A0
	//        _____|_________
	//        |      |      |
	//       z0     y0     x0
	//        |      \..../ .
	//        |       \../  .
	//        |        A1   .
	//        |       .  \  .
	//        |      .    \ .
	//        |     .      L0
	//        |    .
	//        |  L1a
	//        |  /
	//        L1b
	//
	// where . represents a cloud of nodes.

	this.create_cset("A0");

	this.create_cset("x0");
	this.create_cset("x1");
	this.create_cset("x2");

	this.clean_update_to_letter("A0");
	this.create_cset("z0");

	this.clean_update_to_letter("A0");
	this.create_cset("y0");

	this.create_cset("y1a");
	this.clean_update_to_letter("y0");
	this.create_cset("y1b");

	this.merge_with_letter("y1a", "y2a");	// merge y1a into y1b giving y2a

	this.clean_update_to_letter("y0");
	this.create_cset("y1c");
	this.create_cset("y2b");

	this.merge_with_letter("y1b", "y3");	// merge y1b into y2b giving y3

	this.merge_with_letter("y2a", "y4a");	// merge y2a into y3 giving y4a

	this.clean_update_to_letter("y3");
	this.merge_with_letter("x2", "x4");	// merge x2 into y3 giving x4
	this.merge_with_letter("y4a", "x5");	// merge y4a into x4 giving x5
	this.create_cset("x6");

	this.clean_update_to_letter("y3");
	this.create_cset("y4b");

	this.create_cset("y5a");
	this.create_cset("y6a");
	this.create_cset("y7a");

	this.clean_update_to_letter("y4b");
	this.create_cset("y5b");
	this.create_cset("y6b");

	this.merge_with_letter("y5a", "y7b");	// merge y5a into y6b giving y7b

	this.create_cset("A1");

	this.create_cset("w9");
	this.create_cset("w10a");
	this.clean_update_to_letter("w9");
	this.create_cset("w10b");

	this.merge_with_letter("w10a", "w11");	// merge w10a into w10b giving w11

	this.create_cset("L1a");

	this.merge_with_letter("z0", "L1b");	// merge z0 into L1a giving L1b

	this.clean_update_to_letter("x6");
	this.merge_with_letter("y5a", "x7");	// merge y5a into x6 giving x7

	this.create_cset("x8");
	this.create_cset("x9");
	this.create_cset("x10");

	this.merge_with_letter("A1", "L0");	// merge A1 into x10 giving L0

	this.dump_letter_map();

	var expect = new Array();
	expect["LCA" ] = ["A1"];
	expect["LEAF"] = ["L0", "L1a"];
	var success_L0_L1a = this.compute_daglca("L0", "L1a", expect);

	var expect = new Array();
	expect["LCA" ] = ["A1"];
	expect["LEAF"] = ["L0", "L1b"];
	var success_L0_L1b =  this.compute_daglca("L0", "L1b", expect);

	if (success_L0_L1a != success_L0_L1b)
	    print("Observed Bug:");

    }
}
