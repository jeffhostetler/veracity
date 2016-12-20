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

function st_named_branches_02()
{
    this.no_setup = true;

    this.test_2 = function test_2() 
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

        // -------- create a file in r2 new branch
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "e" });

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "master");

        o = sg.exec(vv, "branch", "new", "foo");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");

        my_create_file("f2.txt", 51);

        vscript_test_wc__add("f2.txt");

		vscript_test_wc__commit( "ok");
        
        testlib.existsOnDisk("f2.txt");

        o = sg.exec(vv, "branch");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.trim() == "foo");

        // -------- pull from r2
        sg.fs.cd(r2_path);

        o = sg.exec(vv, "pull");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "branch", "list");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        testlib.ok(o.stdout.search("needs merge") < 0);
    }
}

