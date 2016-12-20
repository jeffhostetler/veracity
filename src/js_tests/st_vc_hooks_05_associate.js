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

function st_vc_hook_associate()
{
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

    this.testAssociateLocal = function test() 
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

			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			o = sg.exec(vv, "commit", "-m", "ok", "-a", recid);
			testlib.equal(0, o.exit_status, "commit exit status");

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

			if (! testlib.ok(!! matches, "no revision seen"))
				return;

			var csid = matches[1];

			var recs = db.query('vc_changeset', ['*']);

			if (testlib.equal(1, recs.length, "association count"))
			{
				var rec = recs[0];

				testlib.equal(rec.commit, csid, "association changeset");
				testlib.equal(recid, rec.item, "association item");
			}
		}
		finally
		{
			repo.close();
		}
    };

    this.testAssociateOtherRepo = function test() 
    {
		var bugRepoName = this.newRepoAndUser("ringo");
		var commitRepoName = this.newRepoAndUser("pete");   // leaves us in its working dir

		var repo2 = sg.open_repo(commitRepoName);
		var commitrepoid = repo2.repo_id;
		repo2.close();

        var repo = sg.open_repo(bugRepoName);

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

			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			var assocID = bugRepoName + ":" + recid;

			o = sg.exec(vv, "commit", "-m", "ok", "-a", assocID);
			if (! testlib.equal(0, o.exit_status, "commit exit status"))
			{
				testlib.log(o.stdout);
				testlib.log(o.stderr);
			}

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

			if (! testlib.ok(!! matches, "no revision seen"))
				return;

			var csid = matches[1];

			var recs = db.query('vc_changeset', ['*']);

			if (testlib.equal(1, recs.length, "association count"))
			{
				var rec = recs[0];

				var csAssocId = commitrepoid + ":" + csid;

				testlib.equal(rec.commit, csAssocId, "association changeset");
				testlib.equal(recid, rec.item, "association item");
			}
		}
		finally
		{
			repo.close();
		}
    };

	// check associate by ID
    this.testAssociateLocalID = function() 
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

			var rec = db.get_record('item', recid);
			var id = rec.id;

			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			o = sg.exec(vv, "commit", "-m", "ok", "-a", id);
			testlib.equal(0, o.exit_status, "commit exit status");

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

			if (! testlib.ok(!! matches, "no revision seen"))
				return;

			var csid = matches[1];

			var recs = db.query('vc_changeset', ['*']);

			if (testlib.equal(1, recs.length, "association count"))
			{
				var rec = recs[0];

				testlib.equal(rec.commit, csid, "association changeset");
				testlib.equal(recid, rec.item, "association item");
			}
		}
		finally
		{
			repo.close();
		}
    };


	// check right error message on bad bug ID
    this.testAssociateBadId = function()
    {
		var r1_name = this.newRepoAndUser("ringo");

        var repo = sg.open_repo(r1_name);

		if (! testlib.ok(!! repo, "opening repo"))
			return;

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			o = sg.exec(vv, "commit", "-m", "ok", "-a", "1234");
			testlib.equal(1, o.exit_status, "commit exit status");

			testlib.ok(o.stderr.match(/Can't find work item 1234/), 'no-work-item error expected');

			o = sg.exec(vv, "status");

			testlib.ok(o.stdout.match(/[Aa]dded/), "commit should have failed");
		}
		finally
		{
			repo.close();
		}
    };

    this.testAssociateCaseInsensitive = function test() 
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

			// by recid, capitalized
			var assocrecid = recid.toUpperCase();
			testlib.ok(assocrecid != recid);

			var fn = sg.gid();
			sg.file.write(fn, "yo");
			vscript_test_wc__add(fn);

			o = sg.exec(vv, "commit", "-m", "ok", "-a", assocrecid);
			if (! testlib.equal(0, o.exit_status, "commit exit status (recid)"))
			{
				testlib.log(o.stdout);
				testlib.log(o.stderr);
			}

			var rec = db.get_record('item', recid);
			var id = rec.id;
			var assocId = id.toLowerCase();
			testlib.ok(id != assocId);

			fn = sg.gid();
			sg.file.write(fn, "yo");
			vscript_test_wc__add(fn);

			o = sg.exec(vv, "commit", "-m", "ok", "-a", assocId);
			if (! testlib.equal(0, o.exit_status, "commit exit status (ID)"))
			{
				testlib.log(o.stdout);
				testlib.log(o.stderr);
			}
		}
		finally
		{
			repo.close();
		}
    };

    this.testAssociateOtherCaseSensitve = function test() 
    {
		var bugRepoName = this.newRepoAndUser("ringo");
		var commitRepoName = this.newRepoAndUser("pete");   // leaves us in its working dir

		var bugRepoSearchName = bugRepoName.toUpperCase();
		if (! testlib.ok(bugRepoSearchName != bugRepoName, "repo name should contain lowercase chars: " + bugRepoName))
			return;

		var repo2 = sg.open_repo(commitRepoName);
		var commitrepoid = repo2.repo_id;
		repo2.close();

        var repo = sg.open_repo(bugRepoName);

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

			sg.file.write("t.txt", "yo");
			vscript_test_wc__add("t.txt");

			var assocID = bugRepoSearchName + ":" + recid;

			o = sg.exec(vv, "commit", "-m", "ok", "-a", assocID);
			if (! testlib.ok(o.exit_status != 0, "commit should fail"))
			{
				testlib.log(o.stdout);
				testlib.log(o.stderr);
				return;
			}
		}
		finally
		{
			repo.close();
		}
    };

	this.testNoDupOnCommit = function()
	{
		var r2_name = this.newRepoAndUser("george");
		var r1_name = this.newRepoAndUser("ringo");

		var addWI = function(repo)
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			var ztx = db.begin_tx();

			try
			{
				var res = {};
				var rec = ztx.new_record('item');
				res.recid = rec.recid;
				rec.title = sg.gid();
				ztx.commit();
				ztx = null;

				rec = db.get_record('item', res.recid);
				res.id = rec.id;

				return(res);
			}
			finally
			{
				if (ztx)
					ztx.abort();
			}			
		};

		var getAssocs = function(repo, recid)
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			var recs = db.query('vc_changeset', ['*'], "item == '" + recid + "'");

			return(recs);
		};

		var addAndAssoc = function(associations)
		{
			var desc = associations.join(',');

			var fn = sg.gid();
			sg.file.write(fn, fn);
			vscript_test_wc__add(fn);

			var params = ["vv", "commit", "-m", desc];

			for ( var i = 0; i < associations.length; ++i )
			{
				params.push("-a");
				params.push(associations[i]);
			}

			testlib.log("params: " + sg.to_json(params));
			o = sg.exec.apply(this, params);

			testlib.log(o.stdout);
			testlib.log(o.stderr);

			return( testlib.equal(0, o.exit_status, "commit " + desc) );
		}

        var repo1 = sg.open_repo(r1_name);
		var repo2 = sg.open_repo(r2_name);

		try
		{
			// local, both ID
			var rec = addWI(repo1);
			if (testlib.ok(addAndAssoc([rec.id,rec.id]), "add with 2 IDs"))
			{
				var assocs = getAssocs(repo1, rec.recid);
				testlib.equal(1, assocs.length, "count with 2 IDs");
			}

			// local, both recid
			rec = addWI(repo1);
			if (testlib.ok(addAndAssoc([rec.recid,rec.recid]), "add with 2 recids"))
			{
				var assocs = getAssocs(repo1, rec.recid);
				testlib.equal(1, assocs.length, "count with 2 recids");
			}

			// local, one each
			rec = addWI(repo1);
			if (testlib.ok(addAndAssoc([rec.id,rec.recid]), "add with both"))
			{
				var assocs = getAssocs(repo1, rec.recid);
				testlib.equal(1, assocs.length, "count with both");
			}

			// remote, both ID
			rec = addWI(repo2);
			if (testlib.ok(addAndAssoc([r2_name + ":" + rec.id, r2_name + ":" + rec.id]), "add with 2 remote IDs"))
			{
				var assocs = getAssocs(repo2, rec.recid);
				testlib.equal(1, assocs.length, "count with 2 remote IDs");
			}

			// remote, both recid
			rec = addWI(repo2);
			if (testlib.ok(addAndAssoc([r2_name + ":" + rec.recid,r2_name + ":" + rec.recid]), "add with 2 remote recids"))
			{
				var assocs = getAssocs(repo2, rec.recid);
				testlib.equal(1, assocs.length, "count with 2 remote recids");
			}

			// remote, one each
			rec = addWI(repo2);
			if (testlib.ok(addAndAssoc([r2_name + ":" + rec.id, r2_name + ":" + rec.recid]), "add remote with both"))
			{
				var assocs = getAssocs(repo2, rec.recid);
				testlib.equal(1, assocs.length, "count remote with both");
			}

		}
		finally
		{
			repo1.close();
			repo2.close();
		}
		
	};
}
