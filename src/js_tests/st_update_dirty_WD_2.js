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
// Some not-so-basic unit tests for UPDATE.
//
// We build a series of changesets and then add some DIRT to the
// working copy and then make a series of UPDATEs and verify
// that:
// [1] the dirty stuff in the WD is preserved (that the dirt is 
//     rolled-forward into the new/target changeset)
// [2] that non-dirty contents of the WD are updated with the new
//     versions in the target changeset.
//
// The dirt that we create in the WD is INDEPENDENT of the changes
// in the sequence of changesets, so there should not be any 
// conflicts/collisions/issues.
//////////////////////////////////////////////////////////////////

function st_UpdateTestsWithDirtyWD() 
{
    // Helper functions are in update_helpers.js
    // There is a reference list at the top of the file.
    // After loading, you must call initialize_update_helpers(this)
    // before you can use the helpers.
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions

    // this.verbose = true; // add this wherever you want it.  It'll stick through the end of the stGroup.

    this.SG_ERR_UPDATE_CONFLICT = "Conflicts or changes in the working copy prevented UPDATE";	// text of error 192.

    //////////////////////////////////////////////////////////////////

    this.setUp = function()
    {
        this.workdir_root = sg.fs.getcwd();     // save this for later

        // we begin with a clean WD in a fresh REPO.
        // and build a sequence of changesets to use

        // We'll use the sequence from update_helpers.js
        this.generate_test_changesets();
    }

    //////////////////////////////////////////////////////////////////

    this.my_find_unique_path = function(pattern)
    {
	// given simple pattern, such as "@/b3/a3*", find the
	// actual current pathname for the item (which might
	// be of the form "@/b3/a3/~g123456~") such that
	// the caller can pass it to a command that takes an
	// exact name.

	var re = new RegExp( pattern );
	var status = sg.wc.status();
	for (var s_index in status)
	    if (re.test(status[s_index].path))
		return (status[s_index].path);

	return; // undefined
    }

    //////////////////////////////////////////////////////////////////

    this.test_update_dirty_edits_with_revert_all = function()
    {
	vscript_test_wc__print_section_divider("test_update_dirty_edits_with_revert_all");

        if (!this.force_clean_WD())
            return;
        print("/// checkpoint 1");
        if (!this.do_update_when_clean_by_name("Simple ADDs 3"))
            return;
        print("/// checkpoint 2");

        // modify some files that will be removed
        this.do_fsobj_append_file("file_1",                "hello1");
        this.do_fsobj_append_file("dir_1/file_11",         "hello1");
        this.do_fsobj_append_file("dir_1/dir_11/file_111", "hello1");

        var expect_test = new Array;
        expect_test["Modified"] = [ "@/file_1",
                                    "@/dir_1/file_11",
                                    "@/dir_1/dir_11/file_111" ];
        vscript_test_wc__confirm(expect_test);

        // now try to update past the removes

        if (!this.do_update_when_dirty_by_name("Simple DELETEs 1"))
	    return;

	var name_file_1   = "@/file_1";
	var name_file_11  = "@/dir_1/file_11";
	var name_file_111 = "@/dir_1/dir_11/file_111";

        var expect_test = new Array;
        expect_test["Modified"]       = [ name_file_1, name_file_11, name_file_111 ];
        expect_test["Added (Update)"] = [ name_file_1, name_file_11, name_file_111 ];
        expect_test["Unresolved"]     = [ name_file_1, name_file_11, name_file_111 ];
        expect_test["Choice Unresolved (Existence)"] = [ name_file_1, name_file_11, name_file_111 ];
        vscript_test_wc__confirm(expect_test);

	// remove one of them before the revert-all.
	// this should cause an "ADD-SPECIAL + REMOVED" status and the content to be backed up.

	var gid_file_1 = sg.wc.get_item_gid_path( { "src" : name_file_1 } );
	var backup_file_1   = vscript_test_wc__backup_name_of_file( name_file_1, "sg00" );

	vscript_test_wc__remove_np( { "src" : name_file_1, "force" : true } );

        var expect_test = new Array;
        expect_test["Modified"]       = [             name_file_11, name_file_111 ];
        expect_test["Added (Update)"] = [ gid_file_1, name_file_11, name_file_111 ];
        expect_test["Unresolved"]                    = [             name_file_11, name_file_111 ];
        expect_test["Choice Unresolved (Existence)"] = [             name_file_11, name_file_111 ];
	expect_test["Resolved"]                      = [ gid_file_1 ];
	expect_test["Choice Resolved (Existence)"]   = [ gid_file_1 ];
	expect_test["Removed"]        = [ gid_file_1 ];
	expect_test["Ignored"]        = [ "@/" + backup_file_1 ];
        vscript_test_wc__confirm(expect_test);

	// a regular revert-all should create backups of the remaining modified files.

	var backup_file_11  = vscript_test_wc__backup_name_of_file( name_file_11,  "sg00" );
	var backup_file_111 = vscript_test_wc__backup_name_of_file( name_file_111, "sg00" );

	vscript_test_wc__revert_all();

	// after a REVERT-ALL, all of the "ADD-SPECIAL + REMOVED" should 
	// have cancelled out completely.

        var expect_test = new Array;
	expect_test["Ignored"] = [ "@/" + backup_file_1,
				   "@/" + backup_file_11,
				   "@/" + backup_file_111 ];
        vscript_test_wc__confirm(expect_test);

        // clean up after ourselves
        print("/// checkpoint 3");
        if (!this.force_clean_WD())
            return;
    }

    this.test_update_dirty_edits_with_revert_all_in_tx = function()
    {
	vscript_test_wc__print_section_divider("test_update_dirty_edits_with_revert_all_in_tx");

        if (!this.force_clean_WD())
            return;
        print("/// checkpoint 1");
        if (!this.do_update_when_clean_by_name("Simple ADDs 3"))
            return;
        print("/// checkpoint 2");

        // modify some files that will be removed
        this.do_fsobj_append_file("file_1",                "hello1");
        this.do_fsobj_append_file("dir_1/file_11",         "hello1");
        this.do_fsobj_append_file("dir_1/dir_11/file_111", "hello1");

        var expect_test = new Array;
        expect_test["Modified"] = [ "@/file_1",
                                    "@/dir_1/file_11",
                                    "@/dir_1/dir_11/file_111" ];
        vscript_test_wc__confirm(expect_test);

        // now try to update past the removes

        if (!this.do_update_when_dirty_by_name("Simple DELETEs 1"))
	    return;

	var name_file_1   = "@/file_1";
	var name_file_11  = "@/dir_1/file_11";
	var name_file_111 = "@/dir_1/dir_11/file_111";

        var expect_test = new Array;
        expect_test["Modified"]                      = [ name_file_1, name_file_11, name_file_111 ];
        expect_test["Added (Update)"]                = [ name_file_1, name_file_11, name_file_111 ];
        expect_test["Unresolved"]                    = [ name_file_1, name_file_11, name_file_111 ];
        expect_test["Choice Unresolved (Existence)"] = [ name_file_1, name_file_11, name_file_111 ];
        vscript_test_wc__confirm(expect_test);

	// remove one of them before the revert-all.
	// this should cause an "ADD-SPECIAL + REMOVED" status and the content to be backed up.

	var gid_file_1 = sg.wc.get_item_gid_path( { "src" : name_file_1 } );
	var backup_file_1   = vscript_test_wc__backup_name_of_file( name_file_1, "sg00" );

	vscript_test_wc__remove_np( { "src" : name_file_1, "force" : true } );

        var expect_test = new Array;
        expect_test["Modified"]                       = [             name_file_11, name_file_111 ];
        expect_test["Added (Update)"]                 = [ gid_file_1, name_file_11, name_file_111 ];
        expect_test["Unresolved"]                     = [             name_file_11, name_file_111 ];
        expect_test["Choice Unresolved (Existence)"]  = [             name_file_11, name_file_111 ];
	expect_test["Resolved"]                       = [ gid_file_1 ];
	expect_test["Choice Resolved (Existence)"]    = [ gid_file_1 ];
	expect_test["Removed"]                        = [ gid_file_1 ];
	expect_test["Ignored"]                        = [ "@/" + backup_file_1 ];
        vscript_test_wc__confirm(expect_test);

	//////////////////////////////////////////////////////////////////
	// Like the previous test case, but here we do the REVERT-ALL inside
	// of a TX so that we can see what an IN-TX STATUS reports.
	//
	// after a REVERT-ALL, all of the "ADD-SPECIAL + REMOVED" should have
	// completely cancelled out.

	var backup_file_11  = vscript_test_wc__backup_name_of_file( name_file_11,  "sg00" );
	var backup_file_111 = vscript_test_wc__backup_name_of_file( name_file_111, "sg00" );

	var tx = new sg_wc_tx();
	{
	    var result = tx.revert_all();
	    print("TX: REVERT_ALL returned:");
	    print(sg.to_json__pretty_print(result));

	    var status = tx.status();
	    print("TX: STATUS returned:");
	    print(sg.to_json__pretty_print(status));
	    
            var expect_test = new Array;
	    expect_test["Ignored"] = [ "@/" + backup_file_1,
// TODO 2012/07/30 See W0717
//				       "@/" + backup_file_11,
//				       "@/" + backup_file_111
				     ];
            vscript_test_wc__confirm(expect_test, status);

	    tx.apply();
	}

	// post-tx verify we get the same status.

        var expect_test = new Array;
	expect_test["Ignored"] = [ "@/" + backup_file_1,
				   "@/" + backup_file_11,
				   "@/" + backup_file_111 ];
        vscript_test_wc__confirm(expect_test);

        // clean up after ourselves
        print("/// checkpoint 3");
        if (!this.force_clean_WD())
            return;
    }

    this.tearDown = function() 
    {

    }
}
