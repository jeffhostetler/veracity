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

function st_daglca_test_015()
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
	//      |    |
	//    x0e   y0e
	//      |    |
	//    x0d   y0d
	//      |    |
	//    x0c    |
	//      |   A1
	//    x0b   /|
	//      |  / |
	//    x0a /  |
	//      |/   |
	//     x1    |
	//      |    |
	//      |    |
	//     L0    L1

	this.create_cset("A0");
	this.create_cset("y0e");
	this.create_cset("y0d");
	this.create_cset("A1");
	this.create_cset("L1");

	this.clean_update_to_letter("A0");
	this.create_cset("x0");
	this.create_cset("x0e");
	this.create_cset("x0d");
	this.create_cset("x0c");
	this.create_cset("x0b");
	this.create_cset("x0a");

	this.merge_with_letter("A1", "x1");	// merge A1 with x0a creating x1
	this.create_cset("L0");

	this.dump_letter_map();

	var expect = new Array();
	expect["LCA" ] = ["A1"];
	expect["LEAF"] = ["L0", "L1"];
	this.compute_daglca("L0", "L1", expect);
    }
}
