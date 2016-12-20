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

//////////////////////////////////////////////////////////////////

function st_X2827()
{
    this.no_setup = true;

    this.test_1 = function()
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

	sg.vv2.whoami({ "username" : "ringo", "create" : true });

	sg.fs.mkdir("dir1");
        my_create_file("dir1/f0.txt", 51);
        my_create_file("f1.txt", 51);
	vscript_test_wc__addremove();
	var cs0 = vscript_test_wc__commit();

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Try various commands with invalid args.");

	var o = sg.exec(vv, "rename", "notfound.txt", "newname.txt");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_code != 0), "Expect rename to fail.");
	testlib.ok( (o.stderr.indexOf("Attempt to operate on an item which is not under version control: Source 'notfound.txt' (@/notfound.txt") >= 0), "Expect error message.");


	var o = sg.exec(vv, "rename", "f0.txt", "dir1/newname.txt");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_code != 0), "Expect rename to fail.");
	testlib.ok( (o.stderr.indexOf("Invalid argument: The destination of a RENAME must be a simple entryname, not a pathname: 'dir1/newname.txt'") >= 0), "Expect error message.");


	var o = sg.exec(vv, "move", "f1.txt", "dir1/f0.txt");
	print(o.stdout);
	print(o.stderr);
	testlib.ok( (o.exit_code != 0), "Expect move to fail.");
	testlib.ok( (o.stderr.indexOf("A file was found where a directory was expected: '@/dir1/f0.txt' (in path '@/dir1/f0.txt/f1.txt')") >= 0), "Expect error message.");

        var expect_test = new Array;
        vscript_test_wc__confirm_wii(expect_test);

    }

}
