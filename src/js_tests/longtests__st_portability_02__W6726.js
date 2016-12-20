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

    this.no_setup = true;		// do not create an initial REPO and WD.

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	var my_test_dir  = "test_1" + "_" + new Date().getTime();
	var my_test_root = pathCombine(tempDir, my_test_dir);

	if (!sg.fs.exists(my_test_root))
	    this.do_fsobj_mkdir_recursive(my_test_root);
	this.do_fsobj_cd(my_test_root);
	sg.vv2.init_new_repo({"repo":my_test_dir, "hash":"SHA1/160"});
	whoami_testing( my_test_dir );

	this.do_fsobj_mkdir("AAA");
	vscript_test_wc__addremove();
	this.do_commit("Added AAA");

	vscript_test_wc__rename("AAA", "xxx");	// 2-step rename (see W6834)
	vscript_test_wc__rename("xxx", "aaa");

	// Revert back to original changeset (causing the REVERT
	// code to detect and implictly do a 2-step rename
	// to get "aaa" back to "AAA").  See W6726 and SPRAWL-297.

	vscript_test_wc__revert_all("Back to 'Added AAA'");
    }

    this.test_2 = function()
    {
	var my_test_dir  = "test_2" + "_" + new Date().getTime();
	var my_test_root = pathCombine(tempDir, my_test_dir);

	if (!sg.fs.exists(my_test_root))
	    this.do_fsobj_mkdir_recursive(my_test_root);
	this.do_fsobj_cd(my_test_root);
	sg.vv2.init_new_repo({"repo":my_test_dir, "hash":"SHA1/160"});
	whoami_testing( my_test_dir );

	this.do_fsobj_mkdir("BBB");
	this.do_fsobj_mkdir("CCC");
	vscript_test_wc__addremove();
	this.do_commit("Added BBB and CCC");

	vscript_test_wc__rename("BBB", "xxx");
	vscript_test_wc__rename("CCC", "BBB");

	// Revert back to original changeset (causing the REVERT
	// code to detect and park one of the BBB's because of the
	// HARD TRANSIENT collision.
	//
	// the entryname "BBB" is used exactly once in both the
	// source and target versions of the WD, so we DO NOT have
	// a FINAL COLLISION, but we have a REAL TRANSIENT COLLISION.

	vscript_test_wc__revert_all("Back to 'Added BBB and CCC'");
    }

    this.test_3 = function()
    {
	var my_test_dir  = "test_3" + "_" + new Date().getTime();
	var my_test_root = pathCombine(tempDir, my_test_dir);

	if (!sg.fs.exists(my_test_root))
	    this.do_fsobj_mkdir_recursive(my_test_root);
	this.do_fsobj_cd(my_test_root);
	sg.vv2.init_new_repo({"repo":my_test_dir, "hash":"SHA1/160"});
	whoami_testing( my_test_dir );

	this.do_fsobj_mkdir("BBB");
	this.do_fsobj_mkdir("CCC");
	vscript_test_wc__addremove();
	this.do_commit("Added BBB and CCC");

	vscript_test_wc__rename("BBB", "xxx");
	vscript_test_wc__rename("CCC", "bbb");

	// Revert back to original changeset (causing the REVERT
	// code to detect and park one of the BBB's because of the
	// POTENTIAL TRANSIENT collision.
	//
	// the entryname "BBB" is POTENTIALLY the same as "bbb"
	// (on Mac/Windows).  Only one of them is used in the
	// source and target versions of the WD, so we DO NOT have
	// a FINAL COLLISION, but we have a POTENTIAL TRANSIENT
	// COLLISION.
	//
	// before the fixes for W6726 this test would sometimes
	// pass and sometimes fail (depending upon the order of
	// the GIDs (because it would not do any parking)).  with
	// the fix, it should park the "bbb".

	vscript_test_wc__revert_all("Back to 'Added BBB and CCC'");
    }

}
