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

function st_locks__L3436()
{
    this.no_setup = true;

    this.test = function test() 
    {
        // -------- create r1

        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, "r1." + r1_name);
	print("r1_path is: " + r1_path);

        if (!sg.fs.exists(r1_path)) 
            sg.fs.mkdir_recursive(r1_path);
	sg.fs.cd(r1_path);

        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	sg.vv2.whoami({ "username": "ringo", "create" : true });

        my_create_file("f1.txt", 51);
	vscript_test_wc__addremove();
	vscript_test_wc__commit();

	print("After commit in r1 with whoami=" + sg.vv2.whoami());

        // -------- clone r1 r2

        var r2_name = sg.gid();
        var r2_path = pathCombine(tempDir, "r2." + r2_name);
	print("r2_path is: " + r2_path);

        o = sg.exec(vv, "clone", r1_name, r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- checkout r2

        o = sg.exec(vv, "checkout", r2_name, r2_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	print("After checkout in r2 with whoami=" + sg.vv2.whoami());

        // -------- in r2, lock the file

	sg.fs.cd(r2_path);

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	o = sg.exec(vv, "status", "--verbose");
        print(o.stdout);

        var expect_test = new Array;
	expect_test["Locked (by You)"] = [ "@/f1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

        // -------- clone r1 r3

        var r3_name = sg.gid();
        var r3_path = pathCombine(tempDir, "r3." + r3_name);
	print("r3_path is: " + r3_path);

        o = sg.exec(vv, "clone", r1_name, r3_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- checkout r3
        o = sg.exec(vv, "checkout", r3_name, r3_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	print("After checkout in r3 with whoami=" + sg.vv2.whoami());

	// -------- confirm that lock is visible (while we still have the same whoami)

        sg.fs.cd(r3_path);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	o = sg.exec(vv, "status", "--verbose");
        print(o.stdout);

        var expect_test = new Array;
	expect_test["Locked (by You)"] = [ "@/f1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

	// --------- switch to a different whoami

	sg.vv2.whoami({ "username": "john", "create" : true });

	print("After user-create in r3 with whoami=" + sg.vv2.whoami() );

	// -------- confirm that lock is visible (now that we have a different whoami)

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	o = sg.exec(vv, "status", "--verbose");
        print(o.stdout);

        var expect_test = new Array;
	expect_test["Locked (by Others)"] = [ "@/f1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

	// -------- cd back to r2 and try status (we should still be john
	// -------- but this repo doesn't know about john yet).

	sg.fs.cd(r2_path);

	//print("After cd back to r2 with whoami=" + sg.vv2.whoami());

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok( (o.stderr.indexOf("No current user is set.") >= 0), "Expect error viewing locks.");

	o = sg.exec(vv, "status", "--verbose");
        print(o.stdout);

	// Since this repo instance doesn't know about john, all
	// locks gets reported as owned by others.

        var expect_test = new Array;
	expect_test["Locked (by Others)"] = [ "@/f1.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

    }
}

