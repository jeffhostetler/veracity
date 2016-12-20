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

function st_revert_all__P0604()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    //////////////////////////////////////////////////////////////////

    this.test_1 = function()
    {
	this.force_clean_WD();

        this.do_fsobj_create_file("file_0");
        vscript_test_wc__addremove();

        this.do_fsobj_create_file("file_1");
        this.do_fsobj_create_file("file_2");
        this.do_fsobj_mkdir("dir_0");

        var expect_test = new Array;
	expect_test["Added"] = [ "@/file_0" ];
	expect_test["Found"] = [ "@/file_1",
				 "@/file_2",
				 "@/dir_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	vscript_test_wc__commit("Commit1");

	sg.file.append( "file_0", "Hello World!\n" );

        var expect_test = new Array;
	expect_test["Modified"] = [ "@/file_0" ];
	expect_test["Found"]    = [ "@/file_1",
				    "@/file_2",
				    "@/dir_0" ];
        vscript_test_wc__confirm_wii(expect_test);

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("RevertAll");

	var r = sg.wc.revert_all( { "no_backups" : true,
				    "verbose" : true } );
	print(sg.to_json__pretty_print(r));

	testlib.ok( (r.stats.unchanged == 1), "Expect 1 unchanged for @.");
	testlib.ok( (r.stats.changed == 1), "Expect 1 changed for revert content of file_0.");
	testlib.ok( (r.stats.deleted == 0), "Nothing should have been deleted.");

        var expect_test = new Array;
	expect_test["Found"]    = [ "@/file_1",
				    "@/file_2",
				    "@/dir_0" ];
        vscript_test_wc__confirm_wii(expect_test);


    }
}
