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

function st_named_branches_04()
{
    this.no_setup = true;

    this.test_4 = function test_4() 
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

        // -------- clone r1 r2
        var r2_name = sg.gid();
        var r2_path = pathCombine(tempDir, r2_name);

        o = sg.exec(vv, "clone", r1_name, r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- checkout r2
        o = sg.exec(vv, "checkout", r2_name, r2_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- clone r1 r3
        var r3_name = sg.gid();
        var r3_path = pathCombine(tempDir, r3_name);

        o = sg.exec(vv, "clone", r1_name, r3_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- checkout r3
        o = sg.exec(vv, "checkout", r3_name, r3_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- clone r1 r4
        var r4_name = sg.gid();
        var r4_path = pathCombine(tempDir, r4_name);

        o = sg.exec(vv, "clone", r1_name, r4_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- checkout r4
        o = sg.exec(vv, "checkout", r4_name, r4_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

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

        // -------- create a file in r2 master
        sg.fs.cd(r2_path);

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        my_create_file("f2.txt", 52);

        vscript_test_wc__add("f2.txt");

        vscript_test_wc__commit( "ok");

        testlib.existsOnDisk("f2.txt");

        // -------- create a file in r3 master
        sg.fs.cd(r3_path);

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        my_create_file("f3.txt", 53);

        vscript_test_wc__add("f3.txt");

        vscript_test_wc__commit( "ok");

        testlib.existsOnDisk("f3.txt");

        // -------- create a file in r4 master
        sg.fs.cd(r4_path);

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        my_create_file("f4.txt", 54);

        vscript_test_wc__add("f4.txt");

        vscript_test_wc__commit( "ok");

        testlib.existsOnDisk("f4.txt");

        // -------- pull from r2 into r1
        sg.fs.cd(r1_path);

        o = sg.exec(vv, "pull", r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- pull from r3 into r1
        sg.fs.cd(r1_path);

        o = sg.exec(vv, "pull", r3_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- pull from r4
        sg.fs.cd(r4_path);

        o = sg.exec(vv, "pull");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- make sure the branch needs merge now
        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") > 0);
        
        // -------- print out the pile
        var repo = sg.open_repo(r4_name);
        var pile = repo.named_branches();
        repo.close();
        print(sg.to_json__pretty_print(pile));

        // -------- we have 4 heads
        o = sg.exec(vv, "heads");
        testlib.ok(o.exit_status == 0);
        print(o.stdout);

        // -------- collect all 4 leaves
        sg.fs.cd(r1_path);
        var r1_leaf = sg.wc.parents()[0];
        print("r1_leaf: ", r1_leaf);

        sg.fs.cd(r2_path);
        var r2_leaf = sg.wc.parents()[0];
        print("r2_leaf: ", r2_leaf);

        sg.fs.cd(r3_path);
        var r3_leaf = sg.wc.parents()[0];
        print("r3_leaf: ", r3_leaf);

        sg.fs.cd(r4_path);
        var r4_leaf = sg.wc.parents()[0];
        print("r4_leaf: ", r4_leaf);

        // -------- go to r4 and remove the branch
        sg.fs.cd(r4_path);

        o = sg.exec(vv, "branch", "remove-head", "master", "--all");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- make sure the branch is gone
        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "");
        
    }
}

