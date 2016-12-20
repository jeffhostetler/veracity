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

function st_daglca_test_012()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    load("daglca_helpers.js");
    initialize_daglca_helpers(this);

    //////////////////////////////////////////////////////////////////

    this.test_daglca = function()
    {
	//           A
	//          / \.
	//         B   C
	//        / \ / \
	//       D   E   F

	this.create_cset("A");
	this.create_cset("B");
	this.create_cset("D");
	this.clean_update_to_letter("A");
	this.create_cset("C");
	this.create_cset("F");
	this.clean_update_to_letter("B");
	this.merge_with_letter("C", "E");   // merge B with C creating E

	this.dump_letter_map();

	var expect = new Array();
	expect["LEAF"] = ["B", "C"];
	expect["LCA" ] = ["A"];
	this.compute_daglca_n(expect);

	var expect = new Array();
	expect["LEAF"] = ["D", "F"];
	expect["LCA" ] = ["A"];
	this.compute_daglca_n(expect);

	var expect = new Array();
	expect["LEAF"] = ["D", "E", "F"];
 	expect["SPCA"] = ["B", "C"];
	expect["LCA" ] = ["A"];
	this.compute_daglca_n(expect);

    }
}
