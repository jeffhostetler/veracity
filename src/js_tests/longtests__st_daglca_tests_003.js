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

function st_daglca_test_003()
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
	//      B    C
	//     / \  / \.
	//   x0   A1   y0
	//   |   /  \   |
	//   x1 z    w y1
	//   | /      \ |
	//   L0        L1

	this.create_cset("A0");
	this.create_cset("B");
	this.create_cset("x0");
	this.create_cset("x1");
	this.clean_update_to_letter("A0");
	this.create_cset("C");
	this.create_cset("y0");
	this.create_cset("y1");
	this.clean_update_to_letter("B");
	this.merge_with_letter("C", "A1");	// merge B with C creating C1
	this.create_cset("z");
	this.merge_with_letter("x1", "L0");
	this.clean_update_to_letter("A1");
	this.create_cset("w");
	this.merge_with_letter("y1", "L1");

	this.dump_letter_map();

	var expect = new Array();
	expect["LCA" ] = ["A1"];
	expect["LEAF"] = ["L0", "L1"];
	this.compute_daglca("L0", "L1", expect);
    }
}
