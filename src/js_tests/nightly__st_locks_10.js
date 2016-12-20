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
        
        // -------- clone r1 r3
        var r3_name = sg.gid();
        var r3_path = pathCombine(tempDir, r3_name);

        o = sg.exec(vv, "clone", r1_name, r3_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r3_name, r3_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- clone r2 r2b
        var r2b_name = sg.gid();
        var r2b_path = pathCombine(tempDir, r2b_name);

        o = sg.exec(vv, "clone", r2_name, r2b_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r2b_name, r2b_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- clone r3 r3b
        var r3b_name = sg.gid();
        var r3b_path = pathCombine(tempDir, r3b_name);

        o = sg.exec(vv, "clone", r3_name, r3b_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r3b_name, r3b_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- in r2b, lock the file as john
        sg.fs.cd(r2b_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        // -------- in r3b, lock the file as ringo
        sg.fs.cd(r3b_path);

	sg.vv2.whoami({ "username" : "ringo" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        // -------- in r2, push
        sg.fs.cd(r2_path);

        o = sg.exec(vv, "push");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        // -------- in r3, check locks
        sg.fs.cd(r3_path);

        o = sg.exec(vv, "locks", "--pull");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        var expect_test = new Array;
	expect_test["Locked (by You)"] = [ "@/f1.txt" ];
	expect_test["Locked (by Others)"] = [ "@/f1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

        var repo = sg.open_repo(r3_name);
        var locks = repo.get_status_of_version_control_locks(
                {
                }
                );
        print(sg.to_json__pretty_print(locks));
        testlib.ok(("duplicate" in locks));
        repo.close();

        // -------- try to push

        o = sg.exec(vv, "push");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status != 0);

        o = sg.exec(vv, "unlock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        var repo = sg.open_repo(r3_name);
        var locks = repo.get_status_of_version_control_locks(
                {
                }
                );
        print(sg.to_json__pretty_print(locks));
        testlib.ok(!("duplicate" in locks));
        repo.close();

        var expect_test = new Array;
	expect_test["Locked (by Others)"] = [ "@/f1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

    }
}

