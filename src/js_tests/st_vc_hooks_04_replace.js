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

function st_vc_hook_replace()
{
    this.testReplaceAsk = function test() 
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

		var dummyhook = 'function hook(params) { }';

        var repo = sg.open_repo(r1_name);
        repo.install_vc_hook(
                {
                    "interface" : "ask__wit__add_associations", 
                    "js" : dummyhook
                }
                );

        repo.install_vc_hook(
                {
                    "interface" : "broadcast__after_commit", 
                    "js" : dummyhook
                }
                );

		var hook = 'function hook(params) { return( { error: "something" } ); }';

        repo.install_vc_hook(
                {
                    "interface" : "ask__wit__add_associations", 
                    "js" : hook,
					"replace": true
                }
                );
		var db = new zingdb(repo, sg.dagnum.VC_HOOKS);
		var recs = db.query('hook', ['*'], "interface == 'ask__wit__add_associations'");

		testlib.equal(1, recs.length, "only one ask hook record after replace");
		
		recs = db.query('hook', ['*']);
		testlib.equal(4, recs.length, "non-specific hook count after replace");
        repo.close();
    };

	this.testIsInstalled = function()
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

		var dummyhook = 'function hook(params) { }';

        var repo = sg.open_repo(r1_name);

		try
		{
			var installedBefore = repo.vc_hooks_installed('ask__wit__add_associations');

			testlib.equal(1, installedBefore.length, "installed list before installation");

			if (installedBefore.length > 0)
				testlib.equal("scrum", installedBefore[0].module, "old module value");

			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__add_associations", 
                    "js" : dummyhook,
					"module" : "bozo",
					"version": 3,
					"replace": true
                }
            );

			var installedAfter = repo.vc_hooks_installed('ask__wit__add_associations');

			if (testlib.equal(1, installedAfter.length, "installed list after installation"))
			{
				var details = installedAfter[0];
				
				testlib.equal("bozo", details.module, "module name");
				testlib.equal(3, details.version, "version");
			}
		}
		finally
		{
			if (repo)
				repo.close();
		}
	};
	this.newRepoAndUser = function(uname)
	{
		var rname = sg.gid();
		var rpath = pathCombine(tempDir, rname);
		if (! sg.fs.exists(rpath))
			sg.fs.mkdir_recursive(rpath);
		sg.fs.cd(rpath);

        var o = sg.exec(vv, "init", rname, ".");
		if (o.exit_status != 0)
			throw("unable to create repo " + rname);

	sg.vv2.whoami({ "username" : uname, "create" : true });

		return(rname);
	};
  this.testAutoReplace = function()
	{
	
	var r1_name = this.newRepoAndUser("ringo");

        var repo = sg.open_repo(r1_name);

		if (! testlib.ok(!! repo, "opening repo"))
			return;

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			var ztx = db.begin_tx();
			var item = ztx.new_record('item');
			var recid = item.recid;
			item.title = "hi";
			var ct = ztx.commit();

			if (ct.errors)
			{
				var msg = sg.to_json(ct);

				throw (msg);
			}

			var installedBefore = repo.vc_hooks_installed('ask__wit__validate_associations');

			testlib.equal(1, installedBefore.length, "installed list before installation");

			if (installedBefore.length > 0)
				testlib.equal("scrum", installedBefore[0].module, "old module value");

			var dummyhook = 'function hook(params) { }';
			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__validate_associations", 
                    "js" : dummyhook,
					"module" : "scrum",
					"version": 4000,
					"replace": true
                }
            );
			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__validate_associations", 
                    "js" : dummyhook,
					"module" : "scrum",
					"version": 4010,
					"replace": false
                }
            );
			
			var installedAfter = repo.vc_hooks_installed('ask__wit__validate_associations');

			if (testlib.equal(2, installedAfter.length, "installed list after installation"))
			{
				var details = installedAfter[0];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4000, details.version, "version");

				details = installedAfter[1];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4010, details.version, "version");
			}
			print(sg.to_json__pretty_print(installedAfter));
			
			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			o = testlib.execVV( "commit", "-m", "ok", "--assoc", recid);
			//o = testlib.execVV( "commit", "-m", "ok", "-a", recid);
			testlib.equal(0, o.exit_status, "commit exit status");

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

			if (! testlib.ok(!! matches, "no revision seen"))
				return;

			var installedAfter2 = repo.vc_hooks_installed('ask__wit__validate_associations');

			print(sg.to_json__pretty_print(installedAfter2));
			
			if (testlib.equal(1, installedAfter2.length, "installed list after commit"))
			{
				var details = installedAfter2[0];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4010, details.version, "version");
			}
		}
		finally
		{
			repo.close();
		}
	};

	this.testAmbiguousHook__lesser_version = function()
	{
		//this test verifys that an ambiguous hook with a lesser version is
		//not an error.  The latest version should just win.
		var r1_name = this.newRepoAndUser("ringo");

        var repo = sg.open_repo(r1_name);

		if (! testlib.ok(!! repo, "opening repo"))
			return;

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			var ztx = db.begin_tx();
			var item = ztx.new_record('item');
			var recid = item.recid;
			item.title = "hi";
			var ct = ztx.commit();

			if (ct.errors)
			{
				var msg = sg.to_json(ct);

				throw (msg);
			}

			var installedBefore = repo.vc_hooks_installed('ask__wit__validate_associations');

			testlib.equal(1, installedBefore.length, "installed list before installation");

			if (installedBefore.length > 0)
				testlib.equal("scrum", installedBefore[0].module, "old module value");

			var dummyhook = 'function hook(params) { }';
			var dummyhook2 = 'function hook(params) { params; }';
			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__validate_associations", 
                    "js" : dummyhook,
					"module" : "scrum",
					"version": 4,
					"replace": true
                }
            );
			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__validate_associations", 
                    "js" : dummyhook2,
					"module" : "scrum",
					"version": 4,
					"replace": false
                }
			);
			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__validate_associations", 
                    "js" : dummyhook,
					"module" : "scrum",
					"version": 7000,
					"replace": false
                }
            );
			
			var installedAfter = repo.vc_hooks_installed('ask__wit__validate_associations');

			if (testlib.equal(3, installedAfter.length, "installed list after installation"))
			{
				var details = installedAfter[0];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4, details.version, "version");

				details = installedAfter[1];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4, details.version, "version");
				
				details = installedAfter[2];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(7000, details.version, "version");
			}
			print(sg.to_json__pretty_print(installedAfter));
			
			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			o = testlib.execVV( "commit", "-m", "ok", "--assoc", recid);
			//o = testlib.execVV( "commit", "-m", "ok", "-a", recid);
			testlib.equal(0, o.exit_status, "commit exit status");

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

			if (! testlib.ok(!! matches, "no revision seen"))
				return;

			var installedAfter2 = repo.vc_hooks_installed('ask__wit__validate_associations');

			print(sg.to_json__pretty_print(installedAfter2));
			
			if (testlib.equal(1, installedAfter2.length, "installed list after commit"))
			{
				var details = installedAfter2[0];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(7000, details.version, "version");
			}
		}
		finally
		{
			repo.close();
		}
	};	
	
	this.testAmbiguousHook__highest_version = function()
	{
		//this test verifys that an ambiguous hook with a lesser version is
		//not an error.  The latest version should just win.
		var r1_name = this.newRepoAndUser("ringo");

        var repo = sg.open_repo(r1_name);

		if (! testlib.ok(!! repo, "opening repo"))
			return;

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			var ztx = db.begin_tx();
			var item = ztx.new_record('item');
			var recid = item.recid;
			item.title = "hi";
			var ct = ztx.commit();

			if (ct.errors)
			{
				var msg = sg.to_json(ct);

				throw (msg);
			}

			var installedBefore = repo.vc_hooks_installed('ask__wit__validate_associations');

			testlib.equal(1, installedBefore.length, "installed list before installation");

			if (installedBefore.length > 0)
				testlib.equal("scrum", installedBefore[0].module, "old module value");

			var dummyhook = 'function hook(params) { }';
			var dummyhook2 = 'function hook(params) { params; }';
			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__validate_associations", 
                    "js" : dummyhook,
					"module" : "scrum",
					"version": 4000,
					"replace": true
                }
            );
			repo.install_vc_hook(
                {
                    "interface" : "ask__wit__validate_associations", 
                    "js" : dummyhook2,
					"module" : "scrum",
					"version": 4000,
					"replace": false
                }
			);
			
			var installedAfter = repo.vc_hooks_installed('ask__wit__validate_associations');

			if (testlib.equal(2, installedAfter.length, "installed list after installation"))
			{
				var details = installedAfter[0];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4000, details.version, "version");

				details = installedAfter[1];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4000, details.version, "version");
			}
			print(sg.to_json__pretty_print(installedAfter));
			
			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			o = testlib.execVV_expectFailure( "commit", "-m", "ok", "--assoc", recid);
			//o = testlib.execVV( "commit", "-m", "ok", "-a", recid);
			testlib.notEqual(-1, o.stderr.indexOf("multiple times at version 4000"), "commit should fail");

			var installedAfter2 = repo.vc_hooks_installed('ask__wit__validate_associations');

			print(sg.to_json__pretty_print(installedAfter2));
			
			if (testlib.equal(2, installedAfter2.length, "installed list after commit"))
			{
				var details = installedAfter2[0];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4000, details.version, "version");
				
				details = installedAfter2[1];
				
				testlib.equal("scrum", details.module, "module name");
				testlib.equal(4000, details.version, "version");
			}
		}
		finally
		{
			repo.close();
		}
	};	
}
