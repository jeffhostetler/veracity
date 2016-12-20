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

function st_wc_daglca_bitvector_1()
{
    load("update_helpers.js");
    initialize_update_helpers(this);

    load("daglca_helpers.js");
    initialize_daglca_helpers(this);

    //////////////////////////////////////////////////////////////////

    if (testlib.defined.SG_LONGTESTS)
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

	// TODO pick a good value for the height of the lattice.
	// TODO a value of 32 will cause us to cross over 64 bits
	// TODO and trigger all of the word-extension stuff.  i've
	// TODO run this with 200, but that test takes ~20 minutes
	// TODO on my mac.

	var kLimit = 40;
	for (var k=1; k<kLimit; k++)
	{
	    // enter loop with p0 === n[k-1][0] and p1 === n[k-1][1]
	    // and WD set to p1.

	    var m0 = "n-" + k + "-0";
	    var m1 = "n-" + k + "-1";

	    print("");
	    print("//////////////////////////////////////////////////////////////////");
	    print("Beginning level [" + k + "]:");

	    this.merge_unique_with_letter(p0, m0);	// merge(p1,p0) ==> m0
	    this.clean_update_to_letter(p0);
	    this.merge_unique_with_letter(p1, m1);	// merge(p0,p1) ==> m1

	    // leave expect["LCA"] set to A
	    expect["SPCA"].push( p0 );		// accumulate n[0..k-1][0..1] in SPCA
	    expect["SPCA"].push( p1 );		// to reflect criss-cross ladder.
	    expect["LEAF"] = [m0, m1];		// reset leaves to new heads.
	    this.compute_daglca_n(expect);

	    p0 = m0;
	    p1 = m1;
	}

    }
}
