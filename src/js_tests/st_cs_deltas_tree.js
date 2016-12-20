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

function num_keys(obj)
{
    var count = 0;
    for (var prop in obj)
    {
        count++;
    }
    return count;
}

function get_keys(obj)
{
    var count = 0;
    var a = [];
    for (var prop in obj)
    {
        a[count++] = prop;
    }
    return a;
}

function st_cs_deltas_tree_01()
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

        var csid0 = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs0 = repo.fetch_json(csid0);
        print(sg.to_json__pretty_print(cs0));
        repo.close();
        var gid_root = get_keys(cs0.tree.changes[""])[0];

        // setup user
	sg.vv2.whoami({ "username": "ringo", "create" : true });

        // add the file

        my_create_file("f1.txt", 51);
        o = sg.exec(vv, "add", "f1.txt");
        testlib.ok(o.exit_status == 0);

        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        testlib.existsOnDisk("f1.txt");

        var csid1 = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs1 = repo.fetch_json(csid1);
        print(sg.to_json__pretty_print(cs1));
        repo.close();

        testlib.ok("tree" in cs1);
        testlib.ok("changes" in cs1.tree);
        testlib.ok(1 == num_keys(cs1.tree.changes));
        testlib.ok(csid0 in cs1.tree.changes);
        testlib.ok(2 == num_keys(cs1.tree.changes[csid0]));
        var gid_f1;
        for (var g in cs1.tree.changes[csid0])
        {
            if (gid_root != g)
            {
                gid_f1 = g;
                break;
            }
        }
        testlib.ok(4 == num_keys(cs1.tree.changes[csid0][gid_f1]));
        testlib.ok("dir" in cs1.tree.changes[csid0][gid_f1]);
        testlib.ok("hid" in cs1.tree.changes[csid0][gid_f1]);
        testlib.ok("name" in cs1.tree.changes[csid0][gid_f1]);
        testlib.ok("type" in cs1.tree.changes[csid0][gid_f1]);
        testlib.ok("f1.txt" == cs1.tree.changes[csid0][gid_f1].name);
        testlib.ok(1 == cs1.tree.changes[csid0][gid_f1].type);

        // modify the file
        
        sg.file.append("f1.txt", "I was here\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        var csid2 = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs2 = repo.fetch_json(csid2);
        print(sg.to_json__pretty_print(cs2));
        repo.close();

        testlib.ok(1 == num_keys(cs2.tree.changes));
        testlib.ok(csid1 in cs2.tree.changes);
        testlib.ok(2 == num_keys(cs2.tree.changes[csid1]));
        testlib.ok(gid_f1 in cs2.tree.changes[csid1]);
        testlib.ok(1 == num_keys(cs2.tree.changes[csid1][gid_f1]));
        testlib.ok("hid" in cs2.tree.changes[csid1][gid_f1]);

        // create another directory
        
        sg.fs.mkdir("d1");
        o = sg.exec(vv, "add", "d1");
        testlib.ok(o.exit_status == 0);
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        var csid3 = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs3 = repo.fetch_json(csid3);
        print(sg.to_json__pretty_print(cs3));
        repo.close();

        testlib.ok(1 == num_keys(cs3.tree.changes));
        testlib.ok(csid2 in cs3.tree.changes);
        testlib.ok(2 == num_keys(cs3.tree.changes[csid2]));
        testlib.ok(!(gid_f1 in cs3.tree.changes[csid2]));
        var gid_d1;
        for (var g in cs3.tree.changes[csid2])
        {
            if (gid_root != g)
            {
                gid_d1 = g;
                break;
            }
        }

        // modify the file again, 4a-->3
        
        sg.file.append("f1.txt", "added a line again\n");
        o = sg.exec(vv, "commit", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        var csid4a = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs4a = repo.fetch_json(csid4a);
        print(sg.to_json__pretty_print(cs4a));
        repo.close();

        testlib.ok(1 == num_keys(cs4a.tree.changes));
        testlib.ok(csid3 in cs4a.tree.changes);
        testlib.ok(2 == num_keys(cs4a.tree.changes[csid3]));
        testlib.ok(gid_root in cs4a.tree.changes[csid3]);
        testlib.ok(gid_f1 in cs4a.tree.changes[csid3]);
        testlib.ok(1 == num_keys(cs4a.tree.changes[csid3][gid_f1]));
        testlib.ok("hid" in cs4a.tree.changes[csid3][gid_f1]);

        // rename the file, 4b-->3
        
        o = sg.exec(vv, "update", "--detached", "-r", csid3);
	if (o.exit_status != 0)
	{
            print(o.stdout);
            print(o.stderr);
	}
        testlib.ok(o.exit_status == 0);
        testlib.ok(csid3 == sg.wc.parents()[0]);
        o = sg.exec(vv, "rename", "f1.txt", "was_f1");
        testlib.ok(o.exit_status == 0);
        o = sg.exec(vv, "commit", "--detached", "-m", "ok");
	if (o.exit_status != 0)
	{
            print(o.stdout);
            print(o.stderr);
	}
        testlib.ok(o.exit_status == 0);

        var csid4b = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs4b = repo.fetch_json(csid4b);
        print(sg.to_json__pretty_print(cs4b));
        repo.close();

        testlib.ok(1 == num_keys(cs4b.tree.changes));
        testlib.ok(csid3 in cs4b.tree.changes);
        testlib.ok(2 == num_keys(cs4b.tree.changes[csid3]));
        testlib.ok(gid_root in cs4b.tree.changes[csid3]);
        testlib.ok(gid_f1 in cs4b.tree.changes[csid3]);
        testlib.ok(1 == num_keys(cs4b.tree.changes[csid3][gid_f1]));
        testlib.ok("name" in cs4b.tree.changes[csid3][gid_f1]);

        // move the file, 4c-->3
        
        o = sg.exec(vv, "update", "--detached", "-r", csid3);
        testlib.ok(o.exit_status == 0);
        testlib.ok(csid3 == sg.wc.parents()[0]);
        o = sg.exec(vv, "move", "f1.txt", "d1");
        testlib.ok(o.exit_status == 0);
        o = sg.exec(vv, "commit", "--detached", "-m", "ok");
        testlib.ok(o.exit_status == 0);

        var csid4c = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs4c = repo.fetch_json(csid4c);
        print(sg.to_json__pretty_print(cs4c));
        repo.close();

        testlib.ok(1 == num_keys(cs4c.tree.changes));
        testlib.ok(csid3 in cs4c.tree.changes);
        testlib.ok(3 == num_keys(cs4c.tree.changes[csid3]));
        testlib.ok(gid_root in cs4c.tree.changes[csid3]);
        testlib.ok(gid_d1 in cs4c.tree.changes[csid3]);
        testlib.ok(gid_f1 in cs4c.tree.changes[csid3]);
        testlib.ok(1 == num_keys(cs4c.tree.changes[csid3][gid_f1]));
        testlib.ok("dir" in cs4c.tree.changes[csid3][gid_f1]);

        // merge 4a and 4b
        
        o = sg.exec(vv, "update", "--detached", "-r", csid4a);
        testlib.ok(o.exit_status == 0);
        testlib.ok(csid4a == sg.wc.parents()[0]);
        o = sg.exec(vv, "merge", "-r", csid4b);
        testlib.ok(o.exit_status == 0);
        o = sg.exec(vv, "commit", "--detached", "-m", "ok");
        print(o.stdout);
        print(o.stderr);
        testlib.ok(o.exit_status == 0);

        var csid5ab = sg.wc.parents()[0];
        var repo = sg.open_repo(r1_name);
        var cs5ab = repo.fetch_json(csid5ab);
        print(sg.to_json__pretty_print(cs5ab));
        repo.close();

        testlib.ok(2 == num_keys(cs5ab.tree.changes));
        testlib.ok(csid4a in cs5ab.tree.changes);
        testlib.ok(csid4b in cs5ab.tree.changes);

        testlib.ok(2 == num_keys(cs5ab.tree.changes[csid4a]));
        testlib.ok(gid_root in cs5ab.tree.changes[csid4a]);
        testlib.ok(gid_f1 in cs5ab.tree.changes[csid4a]);
        testlib.ok(1 == num_keys(cs5ab.tree.changes[csid4a][gid_f1]));
        testlib.ok("name" in cs5ab.tree.changes[csid4a][gid_f1]);

        testlib.ok(2 == num_keys(cs5ab.tree.changes[csid4b]));
        testlib.ok(gid_root in cs5ab.tree.changes[csid4b]);
        testlib.ok(gid_f1 in cs5ab.tree.changes[csid4b]);
        testlib.ok(1 == num_keys(cs5ab.tree.changes[csid4b][gid_f1]));
        testlib.ok("hid" in cs5ab.tree.changes[csid4b][gid_f1]);

    }
}

