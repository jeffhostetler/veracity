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

function my_create_file(relPath, nlines, text, dontcheck) 
{
    var numlines = nlines || Math.floor(Math.random() * 25) + 1;
    var someText = text || "file contents for version 1 - " + Math.floor(Math.random() * numlines);

    if (!sg.fs.exists(relPath)) 
    {
        sg.file.write(relPath, someText + "\n");
    }

    for (var i = 0; i < numlines; i++) 
    {
        sg.file.append(relPath, someText + "\n");
    }

    if (!dontcheck)
    {
        testlib.existsOnDisk(relPath);
    }

    return relPath;
}

function st_crisscross() 
{
    this.no_setup = true;

    this.test_changesets_should_match = function () 
    {
        var r1name = sg.gid();
        var r2name = sg.gid();
		var testname = 'test_changesets_should_match';

        var topdir = pathCombine(tempDir, testname);
        if (!sg.fs.exists(topdir))
	        sg.fs.mkdir_recursive(topdir);
        var r1dir = pathCombine(topdir, r1name);
        var r2dir = pathCombine(topdir, r2name);

        sg.fs.mkdir_recursive(r1dir);
        my_create_file(pathCombine(r1dir, "f1"), 39);
        
        // cd into r1dir
        sg.fs.cd(r1dir);
        
        // vv init
        print("-------- create repo, add f1 --------");
        o = sg.exec(vv, "init", r1name, ".");
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(o.stdout);
        print(o.stderr);

	sg.vv2.whoami({ "repo" : r1name, "username": "crisscross@tests", "create" : true });

		vscript_test_wc__add("f1");
        
		vscript_test_wc__commit( "add f1");
        
        sg.fs.cd(topdir);
        
        // vv clone
        print("-------- clone --------");
        o = sg.exec(vv, "clone", r1name, r2name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(o.stdout);
        print(o.stderr);
        
        // vv checkout
        print("-------- checkout other repo --------");
        o = sg.exec(vv, "checkout", r2name, r2dir);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(o.stdout);
        print(o.stderr);

        testlib.existsOnDisk(pathCombine(r2dir, "f1"));
        
        // add f2 into r2
        my_create_file(pathCombine(r2dir, "f2"), 10);
        sg.fs.cd(r2dir);

        print("-------- add f2 into r2 --------");
        vscript_test_wc__add("f2");
        
        vscript_test_wc__commit( "add f2");
		
        // add f3 into r1
        my_create_file(pathCombine(r1dir, "f3"), 10);
        sg.fs.cd(r1dir);

        print("-------- add f3 into r1 --------");
        vscript_test_wc__add("f3");
        
        vscript_test_wc__commit( "add f3");
		
        // pull in r1
        sg.fs.cd(r1dir);

        print("-------- pull in r1 --------");
        o = sg.exec(vv, "pull", r2name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(o.stdout);
        print(o.stderr);
        
        // pull in r2
        sg.fs.cd(r2dir);

        print("-------- pull in r2 --------");
        o = sg.exec(vv, "pull", r1name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(o.stdout);
        print(o.stderr);
        
        // merge in r1
        sg.fs.cd(r1dir);

        print("-------- merge in r1 --------");
        testlib.wcMerge();
        
	var csid1 = vscript_test_wc__commit("merge in r1");
        
        o = sg.exec(vv, "dump_json", csid1);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print("csid1: ", csid1);
        print(o.stdout);
        var cs1 = eval('(' + o.stdout + ')');
        print(sg.to_json(cs1));
        
        // merge in r2
        sg.fs.cd(r2dir);

        print("-------- merge in r2 --------");
        testlib.wcMerge();
        
	var csid2 = vscript_test_wc__commit("merge in r2");

        o = sg.exec(vv, "dump_json", csid2);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        //print(sg.to_json(o));
        print("csid2: ", csid2);
        var cs2 = eval('(' + o.stdout + ')');
        print(sg.to_json(cs2));
        
        print("-------- finishing --------");
        o = sg.exec(vv, "dump_json", cs2.parents[0]);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        //print(sg.to_json(o));
        print("parent0:", cs2.parents[0]);
        print(sg.to_json(eval('(' + o.stdout + ')')));
        
        o = sg.exec(vv, "dump_json", cs2.parents[1]);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        //print(sg.to_json(o));
        print("parent1:", cs2.parents[1]);
        print(sg.to_json(eval('(' + o.stdout + ')')));
        
        print("csid1 ", csid1);
        print("csid2 ", csid2);

        testlib.ok(csid1 == csid2, "These two changeset IDs should match");

    }
    
    this.test_changesets_should_match_deeper = function () 
    {
        var r1name = sg.gid();
        var r2name = sg.gid();
		var testname = 'test_changesets_should_match_deeper';
        var topdir = pathCombine(tempDir, testname);
        if (!sg.fs.exists(topdir))
	        sg.fs.mkdir_recursive(topdir);
        var r1dir = pathCombine(topdir, r1name);
        var r2dir = pathCombine(topdir, r2name);

        sg.fs.mkdir_recursive(r1dir);
        my_create_file(pathCombine(r1dir, "f1"), 39);
        
        // cd into r1dir
        sg.fs.cd(r1dir);
        
        // vv init
        print("-------- create repo, add f1 --------");
        o = sg.exec(vv, "init", r1name, ".");
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(sg.to_json(o));

	sg.vv2.whoami({ "repo" : r1name, "username": "crisscross@tests", "create" : true });

        vscript_test_wc__add("f1");
        
        vscript_test_wc__commit( "add f1");
		
        sg.fs.cd(topdir);
        
        // vv clone
        print("-------- clone --------");
        o = sg.exec(vv, "clone", r1name, r2name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(sg.to_json(o));
        
        // vv checkout
        print("-------- checkout other repo --------");
        o = sg.exec(vv, "checkout", r2name, r2dir);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(sg.to_json(o));

        testlib.existsOnDisk(pathCombine(r2dir, "f1"));
        
        // add f2 into r2
        my_create_file(pathCombine(r2dir, "f2"), 10);
        sg.fs.cd(r2dir);

        print("-------- add f2 into r2 --------");
        vscript_test_wc__add("f2");
        
        vscript_test_wc__commit( "add f2");
		
        // add f4 into r2
        my_create_file(pathCombine(r2dir, "f4"), 10);
        sg.fs.cd(r2dir);

        print("-------- add f4 into r2 --------");
        vscript_test_wc__add("f4");
        
        vscript_test_wc__commit( "add f4");
		
        // add f3 into r1
        my_create_file(pathCombine(r1dir, "f3"), 10);
        sg.fs.cd(r1dir);

        print("-------- add f3 into r1 --------");
        vscript_test_wc__add("f3");
        
        vscript_test_wc__commit( "add f3");
		
        // add f5 into r1
        my_create_file(pathCombine(r1dir, "f5"), 10);
        sg.fs.cd(r1dir);

        print("-------- add f5 into r1 --------");
        vscript_test_wc__add("f5");
        
        vscript_test_wc__commit( "add f5");
		
        // pull in r1
        sg.fs.cd(r1dir);

        print("-------- pull in r1 --------");
        o = sg.exec(vv, "pull", r2name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(sg.to_json(o));
        
        // pull in r2
        sg.fs.cd(r2dir);

        print("-------- pull in r2 --------");
        o = sg.exec(vv, "pull", r1name);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print(sg.to_json(o));
        
        // merge in r1
        sg.fs.cd(r1dir);

        print("-------- merge in r1 --------");
        testlib.wcMerge();
        
	var csid1 = vscript_test_wc__commit("merge in r1");
        
        o = sg.exec(vv, "dump_json", csid1);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        print("csid1: ", csid1);
        //print(sg.to_json(o));
        var cs1 = eval('(' + o.stdout + ')');
        print(sg.to_json(cs1));
        
        // merge in r2
        sg.fs.cd(r2dir);

        print("-------- merge in r2 --------");
        testlib.wcMerge();
        
	var csid2 = vscript_test_wc__commit("merge in r2");

        o = sg.exec(vv, "dump_json", csid2);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        //print(sg.to_json(o));
        print("csid2: ", csid2);
        var cs2 = eval('(' + o.stdout + ')');
        print(sg.to_json(cs2));
        
        print("-------- finishing --------");
        o = sg.exec(vv, "dump_json", cs2.parents[0]);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        //print(sg.to_json(o));
        print("parent0:", cs2.parents[0]);
        print(sg.to_json(eval('(' + o.stdout + ')')));
        
        o = sg.exec(vv, "dump_json", cs2.parents[1]);
        testlib.ok(o.exit_status == 0, "exit status should be 0");
        //print(sg.to_json(o));
        print("parent1:", cs2.parents[1]);
        print(sg.to_json(eval('(' + o.stdout + ')')));
        
        print("csid1 ", csid1);
        print("csid2 ", csid2);

        testlib.ok(csid1 == csid2, "These two changeset IDs should match");

    }
}

