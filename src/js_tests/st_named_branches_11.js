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

function my_create_file(relPath, nlines) 
{
    var numlines = nlines || Math.floor(Math.random() * 25) + 1;
    var someText = "file contents for version 1 - " + Math.floor(Math.random() * numlines);

    if (!sg.fs.exists(relPath)) 
    {
        sg.file.write(relPath, someText + "\n");
    }

    for (var i = 0; i < numlines; i++) 
    {
        sg.file.append(relPath, someText + "\n");
    }

    return relPath;

}

function st_named_branches_11()
{
    this.no_setup = true;

    this.test_11 = function test_11() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	sg.vv2.whoami({ "username" : "e", "create" : true });

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        var r1_root = sg.wc.parents()[0];

        // -------- create a file in r1 master
        sg.fs.cd(r1_path);

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        my_create_file("f1.txt", 51);

        vscript_test_wc__add("f1.txt");
		
		vscript_test_wc__commit( "ok");

        testlib.existsOnDisk("f1.txt");

        var leaf_f1 = sg.wc.parents()[0];

        // -------- update back to before
		testlib.updateDetached(r1_root);

        // -------- create a file in r1 foo
        sg.fs.cd(r1_path);

        o = sg.exec(vv, "branch", "new", "foo");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");

        my_create_file("f2.txt", 57);

        vscript_test_wc__add("f2.txt");
		
		vscript_test_wc__commit( "ok");
        
        testlib.existsOnDisk("f2.txt");

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");

        var leaf_f2 = sg.wc.parents()[0];

        // -------- update back to before
		testlib.updateDetached(r1_root);

        // -------- create a file untied
        sg.fs.cd(r1_path);

        o = sg.exec(vv, "branch", "detach");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "");

        my_create_file("f3.txt", 57);

        vscript_test_wc__add("f3.txt");
		
        testlib.wcCommitDetached(null, "ok");
		
        testlib.existsOnDisk("f3.txt");

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "");

        var leaf_f3 = sg.wc.parents()[0];

        // -------- switch to master
        sg.fs.cd(r1_path);

		testlib.updateBranch("master");


// confirm update set branch
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");



        testlib.ok(leaf_f1 == sg.wc.parents()[0]);

        testlib.existsOnDisk("f1.txt");
        testlib.notOnDisk("f2.txt");
        testlib.notOnDisk("f3.txt");

        // -------- switch to foo
        sg.fs.cd(r1_path);

		testlib.updateBranch("foo");
        
// confirm update set branch
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");


        testlib.ok(leaf_f2 == sg.wc.parents()[0]);

        testlib.notOnDisk("f1.txt");
        testlib.existsOnDisk("f2.txt");
        testlib.notOnDisk("f3.txt");

        // -------- checkout master
        sg.fs.cd(tempDir);

        var other_wd_name = sg.gid();
        var other_wd_path = pathCombine(tempDir, other_wd_name);

        o = sg.exec(vv, "checkout", r1_name, other_wd_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        sg.fs.cd(other_wd_path);


// confirm checkout set branch
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");


        testlib.ok(leaf_f1 == sg.wc.parents()[0]);

        testlib.existsOnDisk("f1.txt");
        testlib.notOnDisk("f2.txt");
        testlib.notOnDisk("f3.txt");

        // -------- checkout foo
        sg.fs.cd(tempDir);

        var other_wd_name = sg.gid();
        var other_wd_path = pathCombine(tempDir, other_wd_name);

        o = sg.exec(vv, "checkout", r1_name, other_wd_path, "-b", "foo");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        sg.fs.cd(other_wd_path);


// confirm checkout set branch
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");


        testlib.ok(leaf_f2 == sg.wc.parents()[0]);

        testlib.notOnDisk("f1.txt");
        testlib.existsOnDisk("f2.txt");
        testlib.notOnDisk("f3.txt");

        // -------- switch the main wd to master
        sg.fs.cd(r1_path);

		testlib.updateBranch("master");


// confirm update set branch
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");


        testlib.ok(leaf_f1 == sg.wc.parents()[0]);

        testlib.existsOnDisk("f1.txt");
        testlib.notOnDisk("f2.txt");
        testlib.notOnDisk("f3.txt");

        // -------- put the master branch in a needs merge state
        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") < 0);

        o = sg.exec(vv, "branch", "add-head", "master", "-r", leaf_f3);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") > 0);

        // -------- checkout master should fail now
        sg.fs.cd(tempDir);

        var other_wd_name = sg.gid();
        var other_wd_path = pathCombine(tempDir, other_wd_name);

        o = sg.exec(vv, "checkout", r1_name, other_wd_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status != 0);

        // -------- merge
        sg.fs.cd(r1_path);

		testlib.wcMerge();

        testlib.existsOnDisk("f3.txt");

        vscript_test_wc__commit( "ok");

        var leaf_merged = sg.wc.parents()[0];

        // -------- make sure the master branch no longer needs merge
        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") < 0);
        
        // -------- checkout master
        sg.fs.cd(tempDir);

        var other_wd_name = sg.gid();
        var other_wd_path = pathCombine(tempDir, other_wd_name);

        o = sg.exec(vv, "checkout", r1_name, other_wd_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        sg.fs.cd(other_wd_path);

        testlib.ok(leaf_merged == sg.wc.parents()[0]);

        testlib.existsOnDisk("f1.txt");
        testlib.notOnDisk("f2.txt");
        testlib.existsOnDisk("f3.txt");

        // -------- put the foo branch in a needs merge state from outside a WD
        sg.fs.cd(tempDir);

        o = sg.exec(vv, "branch", "list", "--repo", r1_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") < 0);

        o = sg.exec(vv, "branch", "add-head", "foo", "-r", leaf_f3, "--repo", r1_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch", "list", "--repo", r1_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") > 0);

        // -------- switch to foo
        sg.fs.cd(r1_path);

		//LOOK OUT!//This is expected to fail.
        o = sg.exec(vv, "update", "-b", "foo");
        print(sg.to_json(o));
        testlib.ok(o.exit_status != 0);

        // -------- remove one head of foo
        o = sg.exec(vv, "branch", "remove-head", "foo", "-r", leaf_f3);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") < 0);

    }
}

