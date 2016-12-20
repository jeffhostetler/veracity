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

function st_dirty_update_W2237()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        //////////////////////////////////////////////////////////////////
        // fetch the HID of the initial commit.

        this.csets.push( sg.wc.parents()[0] );
        this.names.push( "*Initial Commit*" );

        //////////////////////////////////////////////////////////////////

	this.do_fsobj_mkdir("a");
	this.do_fsobj_mkdir("b");
	this.do_fsobj_create_file("b/file1.txt");
	this.do_fsobj_create_file("b/file2.txt");
	this.do_fsobj_create_file("b/file4.txt");
//	this.do_fsobj_create_file("b/file5.txt");
//	this.do_fsobj_create_file("b/file6.txt");
//	this.do_fsobj_create_file("b/file7.txt");
//	this.do_fsobj_create_file("b/file8.txt");
//	this.do_fsobj_create_file("b/file9.txt");
//	this.do_fsobj_create_file("b/file10.txt");
//	this.do_fsobj_create_file("b/file11.txt");
//	this.do_fsobj_create_file("b/file12.txt");
//	this.do_fsobj_create_file("b/file13.txt");
//	this.do_fsobj_create_file("b/file14.txt");
//	this.do_fsobj_create_file("b/file15.txt");
//	this.do_fsobj_create_file("b/file16.txt");
//	this.do_fsobj_create_file("b/file17.txt");
//	this.do_fsobj_create_file("b/file18.txt");
//	this.do_fsobj_create_file("b/file19.txt");
//	this.do_fsobj_create_file("b/file20.txt");
        vscript_test_wc__addremove();
        this.do_commit("S0");
	this.cset0 = sg.wc.parents()[0];

	this.do_fsobj_append_file("b/file1.txt", "hello again");
	vscript_test_wc__remove_file("b/file2.txt");
	this.do_fsobj_create_file("b/file3.txt");
	vscript_test_wc__move("b", "a");
	vscript_test_wc__rename("a", "a1");
        vscript_test_wc__addremove();
        this.do_commit("S1");
	this.cset1 = sg.wc.parents()[1];
    }

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("create dirt");

	this.do_fsobj_create_file("a1/b/dirt~sg00~");
//	this.do_fsobj_create_file("a1/b/dirt~sg01~");
//	this.do_fsobj_create_file("a1/b/dirt~sg02~");
//	this.do_fsobj_create_file("a1/b/dirt~sg03~");
//	this.do_fsobj_create_file("a1/b/dirt~sg04~");
//	this.do_fsobj_create_file("a1/b/dirt~sg05~");
//	this.do_fsobj_create_file("a1/b/dirt~sg06~");
//	this.do_fsobj_create_file("a1/b/dirt~sg07~");
//	this.do_fsobj_create_file("a1/b/dirt~sg08~");
//	this.do_fsobj_create_file("a1/b/dirt~sg09~");

        var expect_test = new Array;
        expect_test["Found"] = [ "@/a1/b/dirt~sg00~",
//				 "@/a1/b/dirt~sg01~",
//				 "@/a1/b/dirt~sg02~",
//				 "@/a1/b/dirt~sg03~",
//				 "@/a1/b/dirt~sg04~",
//				 "@/a1/b/dirt~sg05~",
//				 "@/a1/b/dirt~sg06~",
//				 "@/a1/b/dirt~sg07~",
//				 "@/a1/b/dirt~sg08~",
//				 "@/a1/b/dirt~sg09~"
			       ];
        vscript_test_wc__confirm_wii(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("confirm that the file is ignorable");

        var expect_test = new Array;
        expect_test["Ignored"] = [ "@/a1/b/dirt~sg00~" ];
        vscript_test_wc__confirm(expect_test);

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");

	// Prior to 07b0cc26452497ac79c445b57870de8db1373fb2, This will fail with:
	//     Error 36 (sglib): Assert failed: Object [GID g5408a6e6de55448088340ce80083def0f110f4c6995711e195a8002500da2b78][name file3.txt] occurs in more than once place in this version [ndx 1] of the tree (and Ndx_Pend is not set).
	//
	// This appears to have a GID-order dependency, so it sometimes passes and
	// sometimes fails depending on the GIDs assigned to file*and dirt*, so run
	// it several times.

	var st = sg.exec(vv, "status", "--rev", this.cset0, "--no-ignores");
	print(dumpObj(st,"status","",0));
	testlib.ok( (st.exit_status==0) );

	print("");
	print("//////////////////////////////////////////////////////////////////");
	print("");
	print("try UPDATE with dirt.");

	try
	{
	    print("Trying update to: " + this.cset0);
	    vscript_test_wc__update( this.cset0 );
	    testlib.ok( (true), "Update succeeded" );
	}
	catch (e)
	{
	    print(dumpObj(e, "catch", "", 0));
	    testlib.ok( (false), "Update failed" );
	}

    }
}
