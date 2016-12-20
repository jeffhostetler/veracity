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

function st_daglca_test_005()
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
	//      t0   b0
	//      |    |
	//      t1   b1
	//      | \  |
	//      |  \ |
	//      |   \|
	//      |    p0
	//      |    |
	//      t2   b2
	//      | \  |
	//      |  \ |
	//      |   \|
	//      |    p1
	//      |    |
	//      t3   b3
	//
	// start a branch 'b' and occasionally pull from trunk 't'

	this.create_cset("A0");
	this.create_cset("t0");
	this.create_cset("t1");
	this.create_cset("t2");
	this.create_cset("t3");

	this.clean_update_to_letter("A0");
	this.create_cset("b0");
	this.create_cset("b1");

	this.merge_with_letter("t1", "p0");	// merge b1 with t1 creating p0

	this.create_cset("b2");

	this.merge_with_letter("t2", "p1");	// merge b2 with t2 creating p1

	this.create_cset("b3");

	this.dump_letter_map();

	var expect = new Array();
	expect["LCA" ] = ["t2"];
	expect["LEAF"] = ["t3", "b3"];
	this.compute_daglca("t3", "b3", expect);

    }
}
