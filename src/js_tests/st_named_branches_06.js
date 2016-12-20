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

function my_count(x)
{
    var c = 0;

    for (var z in x)
    {
        c++;
    }

    return c;
}

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

function st_named_branches_06()
{
    this.no_setup = true;

    this.test_6 = function test_6() 
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

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        var r1_root = sg.wc.parents()[0];

        // -------- untie
        
        o = sg.exec(vv, "branch", "detach");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- create a file in r1 
        sg.fs.cd(r1_path);

        my_create_file("f1.txt", 51);

        vscript_test_wc__add("f1.txt");

        testlib.wcCommitDetached(null, "ok");

        testlib.existsOnDisk("f1.txt");

        var r1_leaf1 = sg.wc.parents()[0];


// confirm that commit --deatched left us untied.
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "");


        // -------- update back to before

        o = sg.exec(vv, "update", "-r", r1_root);
        testlib.ok(o.exit_status == 0);

        testlib.notOnDisk("f1.txt");


// confirm that update to a head implicitly attached us 
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        // -------- untie
        
        o = sg.exec(vv, "branch", "detach");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- create another file in r1 
        sg.fs.cd(r1_path);

        my_create_file("fb.txt", 51);

        vscript_test_wc__add("fb.txt");

		testlib.wcCommitDetached(null, "ok");

        testlib.existsOnDisk("fb.txt");

        var r1_leaf2 = sg.wc.parents()[0];


// confirm that commit --deatched left us untied.
        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "");


        // -------- update back to leaf1

        o = sg.exec(vv, "update", "-r", r1_root);
        testlib.ok(o.exit_status == 0);

        testlib.notOnDisk("f1.txt");

        o = sg.exec(vv, "update", "--detached", "-r", r1_leaf1);
        testlib.ok(o.exit_status == 0);

        testlib.existsOnDisk("f1.txt");

        // -------- put master back
        o = sg.exec(vv, "branch", "move-head", "master", "-r", r1_leaf1);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        print("-------- r1_root: ", r1_root);
        print("-------- pile before");
        var repo = sg.open_repo(r1_name);
        var pile = repo.named_branches();
        repo.close();
        print(sg.to_json__pretty_print(pile));

        testlib.ok(my_count(pile.branches.master.values) == 1);

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        // -------- now put master on the other leaf
        o = sg.exec(vv, "branch", "add-head", "master", "-r", r1_leaf2);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- make sure the branch needs merge now
        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") > 0);
        
    }
}

