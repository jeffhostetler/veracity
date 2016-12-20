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
// do some tricky directory moves grandparent/grandchild and see if
// UPDATE can cope. 
//////////////////////////////////////////////////////////////////

function st_update_cyclic_paths__570() 
{
    // Helper functions are in update_helpers.js
    // There is a reference list at the top of the file.
    // After loading, you must call initialize_update_helpers(this)
    // before you can use the helpers.
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    // this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
	// we begin with a clean WD in a fresh REPO.

	//////////////////////////////////////////////////////////////////
	// fetch the HID of the initial commit.

        this.csets.push( sg.wc.parents()[0] );
	this.names.push( "*Initial Commit*" );
	this.ndxCur = 0;

	//////////////////////////////////////////////////////////////////

	this.do_fsobj_mkdir("dir_1");
	this.do_fsobj_mkdir("dir_1/dir_2");
	this.do_fsobj_mkdir("dir_3");
	vscript_test_wc__addremove();
	this.do_commit("[1]");

	testlib.existsOnDisk("dir_1");
	testlib.existsOnDisk("dir_1/dir_2");
	testlib.existsOnDisk("dir_3");

	vscript_test_wc__move("dir_1/dir_2", "dir_3");
	this.do_commit("[2]");

	testlib.existsOnDisk("dir_1");
	testlib.existsOnDisk("dir_3/dir_2");
	testlib.existsOnDisk("dir_3");

	vscript_test_wc__move("dir_1", "dir_3/dir_2");
	this.do_commit("[3]");

	testlib.existsOnDisk("dir_3/dir_2/dir_1");
	testlib.existsOnDisk("dir_3/dir_2");
	testlib.existsOnDisk("dir_3");

	this.list_csets("After setUp");

	//////////////////////////////////////////////////////////////////

    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	// we are at the head
	// now bounce between the 2 changesets

	if (!this.force_clean_WD())
	    return;

	var ndxHead = this.csets.length - 1;
	var ndxOlder = this.csets.length - 3;

	// at head
	testlib.existsOnDisk("dir_3/dir_2/dir_1");
	testlib.existsOnDisk("dir_3/dir_2");
	testlib.existsOnDisk("dir_3");

	if (!this.do_update_when_clean(ndxOlder))
	    return;

	this.assert_clean_WD();

	// at older
	testlib.existsOnDisk("dir_1");
	testlib.existsOnDisk("dir_1/dir_2");
	testlib.existsOnDisk("dir_3");

	if (!this.do_update_when_clean(ndxHead))
	    return;

	this.assert_clean_WD();

	// at head
	testlib.existsOnDisk("dir_3/dir_2/dir_1");
	testlib.existsOnDisk("dir_3/dir_2");
	testlib.existsOnDisk("dir_3");

	if (!this.do_update_when_clean(ndxOlder))
	    return;

	this.assert_clean_WD();

	// at older
	testlib.existsOnDisk("dir_1");
	testlib.existsOnDisk("dir_1/dir_2");
	testlib.existsOnDisk("dir_3");

    }

    //////////////////////////////////////////////////////////////////
    //
    //////////////////////////////////////////////////////////////////

    this.tearDown = function() 
    {

    }
}
