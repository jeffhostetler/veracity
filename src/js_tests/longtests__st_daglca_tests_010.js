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

function st_daglca_test_010()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    load("daglca_helpers.js");
    initialize_daglca_helpers(this);

    //////////////////////////////////////////////////////////////////

    this.test_daglca = function()
    {
	//        A0
	//       /  \.
	//     x0    y0
	//      |\  /|
	//      | \/ |
	//    x0a /\ y0a
	//      |/  \|
	//     x1    y1
	//      |\  /|
	//      | \/ |
	//    x1a /\ y1a
	//      |/  \|
	//     x2    y2
	//      |    |
	//     L0    L1
	//
	// a criss-cross

	this.create_cset("A0");
	this.create_cset("x0");
	this.create_cset("x0a");

	this.clean_update_to_letter("A0");
	this.create_cset("y0");
	this.create_cset("y0a");

	this.merge_with_letter("x0", "y1");	// merge x0 with y0a creating y1
	this.create_cset("y1a");

	this.clean_update_to_letter("x0a");
	this.merge_with_letter("y0", "x1");	// merge y0 with x0a creating x1
	this.create_cset("x1a");

	this.merge_with_letter("y1", "x2");	// merge y1 with x1a creating x2
	this.create_cset("L0");

	this.clean_update_to_letter("y1a");
	this.merge_with_letter("x1", "y2");	// merge x1 with y1a creating y2
	this.create_cset("L1");

	this.dump_letter_map();

	var expect = new Array();
	expect["LCA" ] = ["A0"];
	expect["SPCA"] = ["x0", "y0", "x1", "y1"];
	expect["LEAF"] = ["L0", "L1"];
	this.compute_daglca("L0", "L1", expect);
    }
}
