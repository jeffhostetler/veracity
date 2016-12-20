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

// test a branch which needs a merge
//
// test merge of a branch
//
// test update with a branch

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

function st_named_branches_01()
{
    this.no_setup = true;

    this.test_1 = function test_1() 
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

        var csid0 = sg.wc.parents()[0];
        print(csid0);

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        o = sg.exec(vv, "branch");
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        o = sg.exec(vv, "branch", "new", "foo");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch", "list");
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        o = sg.exec(vv, "branch");
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");

        my_create_file("f1.txt", 39);

        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch", "list");
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("master") >= 0);
        testlib.ok(o.stdout.search("foo") >= 0);

        o = sg.exec(vv, "branch");
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");

        testlib.existsOnDisk("f1.txt");

        o = sg.exec(vv, "update", "-b", "master");
        testlib.ok(o.exit_status == 0);
// bogus test when tracing is turned on        testlib.ok(o.stdout.trim() == "");

        testlib.notOnDisk("f1.txt");

        o = sg.exec(vv, "update", "-b", "foo");
        testlib.ok(o.exit_status == 0);
// bogus test when tracing is turned on        testlib.ok(o.stdout.trim() == "");

        testlib.existsOnDisk("f1.txt");

        o = sg.exec(vv, "update", "-b", "master");
        testlib.ok(o.exit_status == 0);
// bogus test when tracing is turned on        testlib.ok(o.stdout.trim() == "");

        my_create_file("f2.txt", 74);

        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("master") >= 0);
        testlib.ok(o.stdout.search("needs merge") < 0);
        testlib.ok(o.stdout.search("foo") >= 0);

        var csid_f2 = sg.wc.parents()[0];

        o = sg.exec(vv, "update", "-r", csid0);
        testlib.ok(o.exit_status == 0);

        testlib.notOnDisk("f1.txt");
        testlib.notOnDisk("f2.txt");

        o = sg.exec(vv, "branch");
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        my_create_file("f3.txt", 51);

        o = sg.exec(vv, "add", "f3.txt");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        testlib.existsOnDisk("f3.txt");

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("master") >= 0);
        testlib.ok(o.stdout.search("needs merge") >= 0);
        testlib.ok(o.stdout.search("foo") >= 0);

        var repo = sg.open_repo(r1_name);
        var pile = repo.named_branches();
        repo.close();
        print(sg.to_json__pretty_print(pile));

        o = sg.exec(vv, "merge", "-r", csid_f2);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status == 0);
        print(o.stdout);

        testlib.existsOnDisk("f2.txt");
        testlib.existsOnDisk("f3.txt");

        var repo = sg.open_repo(r1_name);
        var pile = repo.named_branches();
        repo.close();
        print(sg.to_json__pretty_print(pile));

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("master") >= 0);
        testlib.ok(o.stdout.search("needs merge") < 0);
        testlib.ok(o.stdout.search("foo") >= 0);
    }
}

