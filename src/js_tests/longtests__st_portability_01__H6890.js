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

function st_portability_01()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    // this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	this.do_commit("Added AAA");

	vscript_test_wc__rename("AAA", "xxx");	// 2-step rename (see W6834)
	vscript_test_wc__rename("xxx", "aaa");
	this.do_commit("Renamed AAA to aaa");

	// Update back to previous CSET (causing the UPDATE
	// code to detect and implictly do a 2-step rename
	// to get "aaa" back to "AAA").  See H6890 and SPRAWL-915.

	this.do_update_when_clean_by_name("Added AAA");

    }

}
