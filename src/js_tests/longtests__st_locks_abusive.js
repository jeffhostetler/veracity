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

function my_dump_locks()
{
    indentLine("=> (WC) status/locks:");

    print(sg.to_json__pretty_print(sg.vv2.locks()));
    print(sg.to_json__pretty_print(sg.wc.status()));

    print("vv locks");
    var r = sg.exec(vv, "locks");
    print(r.stdout);

    print("vv status");
    var r = sg.exec(vv, "status");
    print(r.stdout);
}

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

function st_locks_unlock_after_commit()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	sg.vv2.whoami({ "username" : "john", "create" : true });

	my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

        testlib.existsOnDisk("f1.txt");
        testlib.existsOnDisk("f2.txt");

        // -------- clone r1 r2
        var r2_name = sg.gid();
        var r2_path = pathCombine(tempDir, r2_name);

        o = sg.exec(vv, "clone", r1_name, r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r2_name, r2_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "lock", "f2.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));
	testlib.ok(-1 != o.stdout.indexOf("f2.txt"));

	my_dump_locks();

        var expect_test = new Array;
	expect_test["Locked (by You)"] = [ "@/f1.txt",
					   "@/f2.txt" ];
        vscript_test_wc__confirm_wii(expect_test);


	sg.file.append("f1.txt", "I was here\n");
        o = testlib.execVV("commit", "-m", "ok");

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//The file we committed should be unlocked.
	//The other file should still be locked.
	testlib.ok(-1 != o.stdout.indexOf("f2.txt"));
	testlib.ok(-1 == o.stdout.indexOf("f1.txt"));

	my_dump_locks();

        var expect_test = new Array;
	expect_test["Locked (by You)"] = [ "@/f2.txt" ];
        vscript_test_wc__confirm_wii(expect_test);


	//Verify that we can relock the file
        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        var expect_test = new Array;
	expect_test["Locked (by You)"] = [ "@/f1.txt",
					   "@/f2.txt" ];
        vscript_test_wc__confirm_wii(expect_test);

    }
}



function st_locks_relock_and_edit_after_completed_lock()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["ringo", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

        testlib.existsOnDisk("f1.txt");
        testlib.existsOnDisk("f2.txt");

        // -------- clone r1 r2
        var r2_name = sg.gid();
        var r2_path = pathCombine(tempDir, r2_name);

        o = sg.exec(vv, "clone", r1_name, r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r2_name, r2_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "lock", "f2.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));
	testlib.ok(-1 != o.stdout.indexOf("f2.txt"));

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	//The file we committed should be unlocked.
	//The other file should still be locked.
	testlib.ok(-1 != o.stdout.indexOf("f2.txt"));
	testlib.ok(-1 == o.stdout.indexOf("f1.txt"));

	testlib.execVV("push"); //Push up john's completed lock and changeset.
	
	// -------- clone r1 r3
	var r3_name = sg.gid();
	var r3_path = pathCombine(tempDir, r3_name);

	o = sg.exec(vv, "clone", r1_name, r3_name);
	print(sg.to_json(o));
	testlib.ok(o.exit_status == 0);

	o = sg.exec(vv, "checkout", r3_name, r3_path);
	print(sg.to_json(o));
	testlib.ok(o.exit_status == 0);

	// -------- in r3, lock the file, again and edit it 
	sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "ringo" });

	o = sg.exec(vv, "lock", "f1.txt");
	print(o.stdout);
	print(o.stderr);
	testlib.ok(o.exit_status == 0);
	
	o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	//The file we committed should be unlocked.
	//The other file should still be locked.
	testlib.ok(-1 != o.stdout.indexOf("f2.txt"));
	testlib.ok(-1 == o.stdout.indexOf("f1.txt"));

	testlib.execVV("push"); //Push up ringo's completed lock and changeset.
    }
}

function st_locks_change_in_other_branch()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);


	//In r3, modify the locked file,
	//while attached to a different branch.
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

        o = sg.exec(vv, "branch", "new", "otherBranch");
        testlib.ok(o.exit_status == 0);

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	//Verify that a push to r1 succeeds
	o = sg.exec(vv, "push");
        testlib.ok(o.exit_status == 0);
    }
}

function st_locks_change_in_detached_branch()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);


	//In r3, modify the locked file,
	//while attached to a different branch.
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

        o = sg.exec(vv, "branch", "detach");
        testlib.ok(o.exit_status == 0);

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "--detached", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	//Verify that a push to r1 succeeds
	o = sg.exec(vv, "push");
        testlib.ok(o.exit_status == 0);
    }
}

function st_locks_push_locks_without_changeset()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	//this lock command will push up the locks
	//dag to r1, without pushing the changeset.
        o = sg.exec(vv, "lock", "f2.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//In r3, modify the locked file with a different change,
	//and attempt to push it
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

	//It's important that the content change here be different from the above
	//change.  If they're the same, the push will succeed.
	sg.file.append("f1.txt", "I was also here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	//Verify that a push to r1 fails,
	//since the lock should still be open.
	o = sg.exec(vv, "push");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status != 0);
    }
}

function st_locks_push_locks_without_changeset__push_same_change()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	//this lock command will push up the locks
	//dag to r1, without pushing the changeset.
        o = sg.exec(vv, "lock", "f2.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//In r3, modify the locked file with the same change
	// and attempt to push
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

	//It's important that the content change here be the same as the above
	//change.  If they're different, the push will fail.
	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	//Verify that a push to r1 succeeds
	//since the lock should still be open.
	o = sg.exec(vv, "push");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);
    }
}

function st_locks_push_locks__deleted_branch()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	my_dump_locks();

		testlib.ok(-1 != o.stdout.indexOf("f1.txt"));
		
		//delete the master branch entirely
		testlib.execVV("branch", "remove-head", "--all", "master");
		
		//Verify that a push to r1 succeeds,
		//even though there is a lock outstanding on a deleted branch.
		o = testlib.execVV("push");
		
		//We happen to still be attached to the branch with no heads
        o = testlib.execVV("locks");
		o = testlib.execVV("unlock", "f1.txt");
    }
}

function st_locks_push_branch_head_move()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

	var csid0_hid1 = sg.wc.parents()[0];

        testlib.existsOnDisk("f1.txt");

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	var csid1_hid2 = sg.wc.parents()[0];

	sg.file.append("f2.txt", "I was also here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	var csid2_hid2 = sg.wc.parents()[0];

	sg.file.append("f2.txt", "I was also here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

	var csid3_hid2 = sg.wc.parents()[0];

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

        // -------- clone r1 r4
        var r4_name = sg.gid();
        var r4_path = pathCombine(tempDir, r4_name);

        o = sg.exec(vv, "clone", r1_name, r4_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r4_name, r4_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);


        sg.fs.cd(r1_path);
	o = sg.exec(vv, "locks", "--pull", "--server", r2_name);
	print(o.stdout);
        testlib.ok(o.exit_status == 0);
	
	//In r3, move the branch head to a changeset
	//where the locked file has the same HID
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

	o = sg.exec(vv, "locks", "--pull");
	print(o.stdout);
        testlib.ok(o.exit_status == 0);

	//In this repo, we'll move the branch head to another changeset, 
	//where the hid is the same
	o = sg.exec(vv, "branch", "move-head", "master", "--all", "-r", csid2_hid2);
        testlib.ok(o.exit_status == 0);

	o = sg.exec(vv, "locks", "--pull");
	o = sg.exec(vv, "locks");
	print(o.stdout);

	//Verify that a push to r1 succeeds,
	o = sg.exec(vv, "push");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//In r4, move the branch head to a changeset
	//where the locked file has a different HID
        sg.fs.cd(r4_path);

	sg.vv2.whoami({ "username" : "paul" });

	//In this repo, we'll move the branch head to another changeset, 
	//where the hid is different
	o = sg.exec(vv, "branch", "move-head", "master", "--all", "-r", csid0_hid1);

	//Verify that a push to r1 fails,
	o = sg.exec(vv, "push");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status != 0);


    }
}


function st_locks_deleted_files()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//First delete, the locked file
	o = testlib.execVV("delete", "f1.txt");

	//undo the changes
	o = testlib.execVV("revert", "@b/f1.txt");
	o = testlib.execVV("unlock", "f1.txt");


	//Now try to lock a deleted file
	o = testlib.execVV("delete", "f1.txt");
	o = testlib.execVV_expectFailure("lock", "@b/f1.txt");
	o = testlib.execVV_expectFailure("unlock", "@b/f1.txt");
	o = testlib.execVV("revert", "@b/f1.txt");

	//This next block verifies that if you have a 
	//locked file that is deleted, committing the 
	//delete will unlock the file
        o = sg.exec(vv, "lock", "@/f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        // -------- in r3, push a deletion to a locked file
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

	o = testlib.execVV("delete", "f1.txt");
        o = testlib.execVV("commit", "-m", "ok");
        o = testlib.execVV_expectFailure("push");

        // -------- in r2, commit the delete of the locked file
	// Verify that the lock is undone.
        sg.fs.cd(r2_path);
	o = testlib.execVV("delete", "f1.txt");

        o = testlib.execVV("commit", "-m", "ok");

        o = testlib.execVV("locks");
	testlib.ok(-1 == o.stdout.indexOf("f1.txt"));

	my_dump_locks();
    }
}

function st_locks_moved_files()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        sg.fs.mkdir_recursive("dir");
        my_create_file("dir/f2.txt", 51);
        o = sg.exec(vv, "add", "dir/f2.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

        testlib.existsOnDisk("f1.txt");
        testlib.existsOnDisk("dir/f2.txt");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//First move, the locked file
	o = testlib.execVV("move", "f1.txt", "dir");

	//undo the changes
	o = testlib.execVV("revert", "dir/f1.txt");
	o = testlib.execVV("unlock", "f1.txt");


	//Now try to lock a move file
	o = testlib.execVV("move", "f1.txt", "dir");
	o = testlib.execVV("lock", "dir/f1.txt");
	o = testlib.execVV("unlock", "dir/f1.txt");

        sg.fs.cd(r2_path + "/dir");
	o = testlib.execVV("lock", "f1.txt");
	o = testlib.execVV("unlock", "f1.txt");
        sg.fs.cd(r2_path);

	o = testlib.execVV("revert", "dir/f1.txt");

	//This next block verifies that if you have a 
	//locked file that is moved, committing the 
	//move will NOT unlock the file
        o = testlib.execVV("lock", "f1.txt");

	o = testlib.execVV("move", "f1.txt", "dir");

        o = testlib.execVV("commit", "-m", "ok");

        o = testlib.execVV("locks");
	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	my_dump_locks();

	
        // -------- in r2, lock the file
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

	o = testlib.execVV("move", "f1.txt", "dir");
        o = testlib.execVV("commit", "-m", "ok");
        o = testlib.execVV("push");
    }
}

function st_locks_renamed_files()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

	//First rename, the locked file
	o = testlib.execVV("rename", "f1.txt", "newName_f1.txt");

	//undo the changes
	o = testlib.execVV("revert", "newName_f1.txt");
	o = testlib.execVV("unlock", "f1.txt");


	//Now try to lock a rename file
	o = testlib.execVV("rename", "f1.txt", "newName_f1.txt");
	o = testlib.execVV("lock", "newName_f1.txt");
	o = testlib.execVV("unlock", "newName_f1.txt");
	o = testlib.execVV("revert", "newName_f1.txt");

	//This next block verifies that if you have a 
	//locked file that is rename, committing the 
	//rename will NOT unlock the file
        o = testlib.execVV("lock", "f1.txt");

	o = testlib.execVV("rename", "f1.txt", "newName_f1.txt");

        o = testlib.execVV("commit", "-m", "ok");

        o = testlib.execVV("locks");
	testlib.ok(-1 != o.stdout.indexOf("f1.txt"));

	my_dump_locks();

	
        // -------- in r2, lock the file
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

	o = testlib.execVV("rename", "f1.txt", "newName_f1.txt");
        o = testlib.execVV("commit", "-m", "ok");
        o = testlib.execVV("push");
    }
}


function st_locks_found_files()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, create a Found file, and try to lock it
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "john" });

        my_create_file("f2.txt", 51);

        o = testlib.execVV_expectFailure("lock", "f2.txt");
    }
}


function st_locks_unpushed_user()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	sg.vv2.whoami({ "username" : "john", "create" : true });

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);

	sg.vv2.whoami({ "username" : "paul", "create" : true });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        // -------- in r2, pull the locks.
		// You should have a lock for a username that 
		// you didn't add.  The users dag should have been 
		// automatically pushed.
        sg.fs.cd(r3_path);
		
	sg.vv2.whoami({ "username" : "john" });
		
		o = testlib.execVV("locks", "--pull");
		testlib.ok(-1 != o.stdout.indexOf("paul")); //make sure the lock is properly attributed

	my_dump_locks();
    }
}

function st_locks_dont_push_branch_head()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);
		
	sg.vv2.whoami({ "username" : "paul" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);
		
		//Commit a change which should unlock the file
		sg.file.append("f1.txt", "I was here\n");
        o = testlib.execVV("commit", "-m", "ok");

		var newHead = sg.vv2.leaves()[0];
		
		//Reaquire the lock.  This used to push up a the new branch head
		//It doesn't any more.
        o = testlib.execVV("lock", "f1.txt");
        
        // -------- in r1, verify that you don't know anything about the new branch head
        sg.fs.cd(r1_path);
		
		//If the branches dag got pushed, this call will fail.
		o = testlib.execVV("history", "--branch=master");
		
		testlib.ok(-1 == o.stdout.indexOf(newHead)); //The lock server shouldn't know anything about the new branch head.

		o = testlib.execVV("locks");
		
		testlib.notEqual(-1, o.stdout.indexOf("f1.txt")); //The file should be locked.

	my_dump_locks();
		
        // -------- in r2, unlock the file
        sg.fs.cd(r2_path);
		
		// unlock the file.  This used to push the branch info, but it shouldn't anymore.
		o = testlib.execVV("unlock", "f1.txt");
		
		// -------- in r1, verify that you don't know anything about the new branch head
        sg.fs.cd(r1_path);
		
		//If the branches dag got pushed, this call will fail.
		o = testlib.execVV("history", "--branch=master");
		
		testlib.ok(-1 == o.stdout.indexOf(newHead)); //The lock server shouldn't know anything about the new branch head.

		o = testlib.execVV("locks");
		
		testlib.notEqual(-1, o.stdout.indexOf("waiting for changeset")); //The locks message should mention that we are waiting for a changeset.
		//The file should still be locked, since we don't have the 
		//changeset that completed the lock (newHead).
		testlib.notEqual(-1, o.stdout.indexOf("f1.txt")); 

	my_dump_locks();
	}
}


function st_locks_multiple_open_or_waiting_locks()
{
    this.no_setup = true;

    this.test = function test() 
    {
        var r1_name = sg.gid();
        var r1_path = pathCombine(tempDir, r1_name);
	vscript_test_wc__print_section_divider("Creating: " + r1_path);
        if (!sg.fs.exists(r1_path)) {
          
            sg.fs.mkdir_recursive(r1_path);
        }
        sg.fs.cd(r1_path);
        
        // -------- create r1
        o = sg.exec(vv, "init", r1_name, ".");
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

	createUsers(["paul", "john"]);

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = testlib.execVV("commit", "-m", "ok");

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

        // -------- in r2, lock the file
        sg.fs.cd(r2_path);
		
	sg.vv2.whoami({ "username" : "paul" });

        o = sg.exec(vv, "lock", "f1.txt");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);
		
		//Commit a change which should unlock the file
		sg.file.append("f1.txt", "I was here\n");
        o = testlib.execVV("commit", "-m", "ok");

		var newHead = sg.vv2.leaves()[0];
		
		//Reaquire the lock.  There is now 1 open lock, and one waiting lock 
		//(the commit hasn't been pushed yet).
        o = testlib.execVV("lock", "f1.txt");
        
		// -------- in r3, make sure that you can handle the multiple open or waiting locks case.
        sg.fs.cd(r3_path);
		
	sg.vv2.whoami({ "username" : "john" });
		
		o = testlib.execVV("locks");
        
		testlib.equal(-1, o.stdout.indexOf("VIOLATED")); 

	my_dump_locks();
		
		//push used to fail with a DUPLICATE LOCK message
		o = testlib.execVV("push");
        
		testlib.equal(-1, o.stdout.indexOf("DUPLICATE")); 

	my_dump_locks();

		my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        testlib.ok(o.exit_status == 0);

		//commit used to fail with a vhash collision error.
        o = testlib.execVV("commit", "-m", "ok");
		
		o = testlib.execVV("lock", "f2.txt");

		//push the new, unrelated changeset up.
		o = testlib.execVV("push");
	}
}
