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

function st_vc_hooks()
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

	sg.vv2.whoami({ "username" : "ringo", "create" : true });

        // -------- install a hook
        var hook_result_filename = sg.gid();
        //var hook_result_path = pathCombine(tempDir, hook_result_filename);
        var hook = "function hook(params) { sg.file.write('";
        hook += hook_result_filename;
        hook += "', sg.to_json__pretty_print(params)); }";

        var repo = sg.open_repo(r1_name);
        repo.install_vc_hook(
                {
                    "interface" : "ask__wit__add_associations", 
                    "js" : hook,
					"replace" : true
                }
                );
		repo.install_vc_hook(
			{
				"interface": "ask__wit__validate_associations",
				"js" : 'function hook() { }',
				"replace": true
			}
		);
        repo.close();

        // now do a commit
        my_create_file("f1.txt", 51);
	vscript_test_wc__add("f1.txt");

        vscript_test_wc__commit( "ok");

        // the assoc hook function should not have been called,
        // so the file should be there
        testlib.notOnDisk(hook_result_filename);

        // now add another file and commit again, WITH --assoc this time
        my_create_file("f2.txt", 81);
        vscript_test_wc__add("f2.txt");

        var comment = "The time for careful analysis is over. It is time for hasty conclusions and finger pointing. Quick, to your Tweeting Stations and flame!";

        o = sg.exec(vv, "commit", "-m", comment, "--assoc", "pyrenean");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        // the hook function should have been called
        testlib.existsOnDisk(hook_result_filename);
        var data = sg.file.read(hook_result_filename);
        print(data);
        var params = eval('(' + data + ')');

        var csid0 = sg.wc.parents()[0];
        testlib.ok(csid0 == params.csid);

        testlib.ok(params.comment == comment);
        testlib.ok(params.branch == "master");
        testlib.ok(params.changeset.dagnum == sg.dagnum.VERSION_CONTROL);
        testlib.ok(params.changeset.parents.length == 1);
        testlib.ok(params.wit_ids.length == 1);
        testlib.ok(params.wit_ids[0] == "pyrenean");
    };

}

