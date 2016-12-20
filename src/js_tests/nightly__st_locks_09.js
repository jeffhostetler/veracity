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

function st_locks_01()
{
    this.no_setup = true;

    this.test = function test() 
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

	createUsers(["john", "paul", "george", "ringo"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        sg.fs.mkdir("d1");
        my_create_file("d1/f2.txt", 31);
        o = sg.exec(vv, "add", "d1/f2.txt");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        testlib.existsOnDisk("f1.txt");

        // -------- clone r1 r2
        var r2_name = sg.gid();
        var r2_path = pathCombine(tempDir, r2_name);

        o = sg.exec(vv, "clone", r1_name, r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r2_name, r2_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);
        
        // -------- in r2, add a file and try to
        // lock it before committing it
        sg.fs.cd(r2_path);

        my_create_file("f3.txt", 51);
        o = sg.exec(vv, "add", "f3.txt");

        o = sg.exec(vv, "lock", "f3.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status != 0);

    }
}

