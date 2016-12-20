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

function st_W9081()
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

        my_create_file("f0.txt", 51);
	vscript_test_wc__addremove();
	var cs0 = vscript_test_wc__commit();

	//////////////////////////////////////////////////////////////////
	vscript_test_wc__print_section_divider("Do status.");

	var tx = new sg_wc_tx();
	var sf0 = tx.get_item_status_flags({ "src":"@/f0.txt", "no_tsc":true });
	testlib.ok( (sf0.isNonDirModified == undefined), "Expect unmodified at sf0.");

	// while holding the TX open, pretend like another process
	// created a new file (in the directory that we have already
	// scanned) and see what happens.

        my_create_file("bogus.txt", 51);

	// we report "isBogus" when it is named explicitly.
	var sf1 = tx.get_item_status_flags({ "src":"@/bogus.txt", "no_tsc":true });
	print(sg.to_json__pretty_print(sf1));
	testlib.ok( (sf1.isBogus == true), "Expect bogus.");

	// we DO NOT include bogus items in a status on the parent directory
	// (since it wasn't present when we called readdir on the directory).
	var s = tx.status();
        var expect_test = new Array;
        vscript_test_wc__confirm(expect_test, s);

    }

}
