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

function st_vc_hook_ask_no()
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
        var o = sg.exec(vv, "init", r1_name, ".");
        testlib.ok(o.exit_status == 0);

	sg.vv2.whoami({ "username" : "ringo", "create" : true });

        // -------- install a hook
        var hook_result_filename = sg.gid();
        //var hook_result_path = pathCombine(tempDir, hook_result_filename);

		var dummyhook = 'function hook(params) { }';

		var hook = 'function hook(params) { return( { error: "something" } ); }';

        var repo = sg.open_repo(r1_name);
        repo.install_vc_hook(
                {
                    "interface" : "ask__wit__add_associations", 
                    "js" : hook,
					"replace": true
                }
                );
        repo.close();

		sg.file.write("f2.txt", "Psychoanalysts at the time suggested Susie's boys were Oedipal wrecks who could hardly peek out from beneath her Freudian slip.");
	vscript_test_wc__add("f2.txt");

        var comment = "Mmmm, floor pie!";

        o = sg.exec(vv, "commit", "-m", comment, "--assoc", "pyrenean");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(0 != o.exit_status, "commit should be non-zero");
    };
}
