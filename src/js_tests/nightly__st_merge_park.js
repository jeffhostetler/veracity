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

function st_merge_park()
{
    load("update_helpers.js");          // load the helper functions
    initialize_update_helpers(this);    // initialize helper functions


    //
    //////////////////////////////////////////////////////////////////

    this.do_update_by_rev = function(rev)
    {
	print("vv update --rev=" + rev);
	var o = sg.exec(vv, "update", "--rev="+rev);

	//print("STDOUT:\n" + o.stdout);
	//print("STDERR:\n" + o.stderr);

	testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	this.assert_current_baseline(rev);
    }

    this.do_exec_merge = function(rev, bExpectFail, msgExpected)
    {
	print("vv merge --rev=" + rev);
	var o = sg.exec(vv, "merge", "--rev="+rev, "--verbose");
	
	print("STDOUT:\n" + o.stdout);
	print("STDERR:\n" + o.stderr);

	if (bExpectFail)
	{
	    testlib.ok( o.exit_status == 1, "Exit status should be 1" );
	    testlib.ok( o.stderr.search(msgExpected) >= 0, "Expecting error message [" + msgExpected + "]. Received:\n" + o.stderr);
	}
	else
	{
	    testlib.ok( o.exit_status == 0, "Exit status should be 0" );
	}
        return o.stdout;
    }

    //////////////////////////////////////////////////////////////////
      /*          In an initial/ancestor (A) checkin create something like:
                
                @/dir_1/dir_2/dir_3/file_1
                @/file_2
                
                create another changeset (B) with an additional file:
                
                @/file_3
                
                Update back to the ancestor.  Delete dir_2 and everything
                under it, commit that (C1).  Then create new dir_2, dir_3, and file_1
                (with different content).  Commit that (C2).
                
                Then with a fresh WD at cset B, merge C2 into it.
                (This should cause B's copy of dir_2, dir_3, and
                file_1 to get parked and then deleted.)  Use the
                --verbose option and confirm that there is a parked
                message for them.  Commit that as M1.
                
                Then with a fresh WD at C2, merge B into it.
                (This should NOT cause any parking because C2's
                copy is going to be preserved.)  Use the --verbose
                option and confirm that no parking is required.
                Commit that as M2.
                
                For extra credit, compare M1 and M2 and confirm
                that they are either identical (csets) or have identical
                contents.       */
                
        // create:
        //
        //      0
        //      |
        //      A      create @/dir_1/dir_2/dir_3/file_1  and  @/file_2
        //     / \
        //    B   |    create @/file_3
        //    |   C1   delete @/dir_1/dir_2
        //    |   |
        //    |   C2   create @/dir_1/dir_2/dir_3/file_1

    //////////////////////////////////////////////////////////////////
    
    this.test_merge_park = function()
    {
	this.force_clean_WD();
        this.do_fsobj_mkdir("dir_1");
        this.do_fsobj_mkdir("dir_1/dir_2");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3");
        this.do_fsobj_create_file_exactly("dir_1/dir_2/dir_3/file_1", "Version 1 first time");
        this.do_fsobj_create_file_exactly("file_2", "Version 1 only time");
        vscript_test_wc__addremove();
        this.do_commit("CSET A");
        
        this.do_fsobj_create_file_exactly("file_3", "Version 1 only time");
        vscript_test_wc__addremove();
        this.do_commit("CSET B");
        this.rev_B = this.csets[ this.csets.length - 1 ];
        
        this.do_update_when_clean_by_name("CSET A");
        vscript_test_wc__remove_dir("dir_1/dir_2");
        vscript_test_wc__addremove();
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("CSET C1");
        
        this.do_fsobj_mkdir("dir_1/dir_2");
        this.do_fsobj_mkdir("dir_1/dir_2/dir_3");
        this.do_fsobj_create_file_exactly("dir_1/dir_2/dir_3/file_1", "Version 1 second time... maybe");
        vscript_test_wc__addremove();
        this.do_commit("CSET C2");
        this.rev_C2 = this.csets[ this.csets.length - 1 ];
        
        this.force_clean_WD();
        this.do_update_when_clean_by_name("CSET B");
        var msg=this.do_exec_merge(this.rev_C2, 0, "");
        vscript_test_wc__addremove();
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("CSET M1");
        this.rev_M1 = this.csets[ this.csets.length - 1 ];
        
        copyFileOnDisk("dir_1/dir_2/dir_3/file_1", "../file_1_M1");
        copyFileOnDisk("file_2", "../file_2_M1");
        copyFileOnDisk("file_3", "../file_3_M1");        
        
// This 'parked' message appears in the verbose journal output.
// In the WC conversion, I changed the format of the journal
// messages, so we won't see this.  The important part is that
// the actual merge succeeded as expected.
// TODO 2012/05/17 think about getting turning this back on.
//        testlib.ok((msg.indexOf("PARKED")>=0),"PARKED should appear in the merge message");
        
        this.force_clean_WD();
        this.do_update_when_clean_by_name("CSET C2");
        var msg2=this.do_exec_merge(this.rev_B, 0, "");
        vscript_test_wc__addremove();
	sg.exec(vv, "branch", "attach", "master");
        this.do_commit("CSET M2");
        this.rev_M2 = this.csets[ this.csets.length - 1 ];
        
        copyFileOnDisk("dir_1/dir_2/dir_3/file_1", "../file_1_M2");
        copyFileOnDisk("file_2", "../file_2_M2");
        copyFileOnDisk("file_3", "../file_3_M2"); 
        
//	testlib.ok((msg2.indexOf("PARKED")<0),"PARKED should not appear in the merge message");
        
        if (this.rev_M1 == this.rev_M2) 
        {
            testlib.ok(true,"Changesets should be equal");
            
        }
        
        else 
        {
            this.do_update_when_clean_by_name("CSET M1");
            print("Confirm objects are in repo at CSET M1 ...");
            testlib.inRepo("dir_1/dir_2/dir_3/file_1");
            testlib.inRepo("file_2");
            testlib.inRepo("file_3");
            
            this.do_update_when_clean_by_name("CSET M2");
            print("Confirm objects are in repo at CSET M2 ...");
            testlib.inRepo("dir_1/dir_2/dir_3/file_1");
            testlib.inRepo("file_2");
            testlib.inRepo("file_3");
        
            testlib.ok(compareFiles("../file_1_M1", "../file_1_M2"), "Files should be identical");
            testlib.ok(compareFiles("../file_2_M1", "../file_2_M2"), "Files should be identical");
            testlib.ok(compareFiles("../file_3_M1", "../file_3_M2"), "Files should be identical");
        
        }
        
    }
    
}
