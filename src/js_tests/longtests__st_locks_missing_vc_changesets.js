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

/* This file tests locks when relevant version control changesets are missing.
 * See K7394 for one concrete example.
 */

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

/* K7394
 * new repo, r1
 * create clones of r1, called r2 and r3 - r2 is the lock coordination repo, r1 and r3 are dev repos
 * commit changeset to r1
 * lock that changeset in r1
 * pull locks from r1 to r2
 * commit a different changeset in r3
 * push from r3 to r2 will fail
 */
function st_locks_K7394()
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

	createUsers(["paul", "john"]);

        // -------- clone r1 to r2
        var r2_name = sg.gid();
        var r2_path = pathCombine(tempDir, r2_name);

        o = sg.exec(vv, "clone", r1_name, r2_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r2_name, r2_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

		// -------- clone r1 to r3
        var r3_name = sg.gid();
        var r3_path = pathCombine(tempDir, r3_name);

        o = sg.exec(vv, "clone", r1_name, r3_name);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "checkout", r3_name, r3_path);
        print(sg.to_json(o));
        testlib.ok(o.exit_status == 0);

		// -------- create changeset in r1
	sg.vv2.whoami({ "username" : "john" });

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        testlib.existsOnDisk("f1.txt");

        // -------- in r1, lock the file
        o = sg.exec(vv, "lock", "f1.txt", "--server", r2_name);
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "locks");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

		// -------- in r3, commit a new, different head for master
        sg.fs.cd(r3_path);

	sg.vv2.whoami({ "username" : "paul" });

        my_create_file("f2.txt", 51);
        o = sg.exec(vv, "add", "f2.txt");
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        testlib.existsOnDisk("f2.txt");

		// -------- pull locks from r1 to r2
        sg.fs.cd(r2_path);
		o = sg.exec(vv, "locks", "--pull", "--server", r1_name);
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

		// -------- pull all from r2 to r3, giving r3 a missing master head and a lock starting on that missing changeset
		sg.fs.cd(r3_path);
		o = sg.exec(vv, "pull", r2_name);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

		// -------- In r3, force master to the one head we have.
		//          This eliminates the warning push would give us about a new head, leaving only the error described in K7394.
		var r3 = sg.open_repo(r3_name);
		try
		{
			var r3_hid_head = r3.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL)[0];
		}
		finally
		{
			r3.close();
		}
		o = sg.exec(vv, "branch", "move-head", "master", "--all", "-r", r3_hid_head);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        // -------- push all (unforced) from r3 to r2
        sg.fs.cd(r3_path);
		o = sg.exec(vv, "push", r2_name);
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);
    };
}

