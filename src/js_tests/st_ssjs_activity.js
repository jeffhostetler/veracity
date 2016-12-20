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

function st_ssjs_activity()
{
	this.setUp = function()
	{
		this.repo = sg.open_repo(repInfo.repoName);

		var serverFiles = sg.local_settings().machine.server.files;
		var core = serverFiles + "/core/";
		var scrum = serverFiles + "/modules/scrum/";
		load(serverFiles + "/dispatch.js");
		load(core + "vv.js");
		load(core + "textCache.js");
		load(scrum + "activity.js");
		load(core + "session.js");
		
		this.username1 = "activity1@sourcegear.com";
		this.username2 = "activity2@sourcegear.com";
		
		this.repo.create_user(this.username1);
		this.repo.create_user(this.username2);
		
		{
			var zs = new zingdb(this.repo, sg.dagnum.USERS);
			var urec = zs.query('user', ['*'], "name == '" + this.username1 + "'");

			this.user1 = urec[0];
			urec = zs.query('user', ['*'], "name == '" + this.username2 + "'");

			this.user2 = urec[0];
			
			urec = zs.query('user', ['*'], "name == '" + repInfo.userName + "'");

			this.userId = urec[0].recid;
		}
	};
	
	this.tearDown = function()
	{
		if (this.repo)
			this.repo.close();
	};
	
	this.testStampedByUsernameInStream = function()
	{
		this.repo.set_user(this.username1);
		var itemTitle = "testStampedByUsernameInStream";
		var item1 = newRecord("item", this.repo, { title: itemTitle });

		newRecord("stamp", this.repo, { name: this.username2, item: item1 });
		
		activity = activityStreams.getActivity({repo:this.repo}, this.user2.recid);
		
		var found = false;
		
		for (var i = 0; i < activity.length; i++)
		{
			if (activity[i].what == itemTitle)
			{
				found = true;
			}
		}
		
		testlib.testResult(found, "Stamped item found in " + this.username2 + "'s activity stream", true, found);
		
		// Ensure it is not appearing in a third users stream
		activity = activityStreams.getActivity({repo:this.repo}, this.userId);
		
		found = false;
		
		for (i = 0; i < activity.length; i++)
		{
			if (activity[i].what == itemTitle)
			{
				found = true;
				break;
			}
		}
		
		testlib.testResult(!found, "Stamped item not found in " + this.userId + "'s activity stream", false, found);
	};

	this.testAddTitleFieldForAtom = function()
	{
		this.repo.set_user(this.username1);
		var itemTitle = "testATFFA";
		var item1 = newRecord("item", this.repo, { title: itemTitle });
		
		var activity = activityStreams.getActivity({repo:this.repo});

		var found = false;
		
		for (var i = 0; i < activity.length; i++)
		{
			if (activity[i].what == itemTitle)
			{
				found = true;

				if (testlib.testResult(!! activity[i].title, "title field in created item"))
					testlib.equal(itemTitle, activity[i].title, "activity title field");
				break;
			}
		}
		
		testlib.testResult(found, "item found in activity stream");

		this.modifyBug(item1, { "priority": "Medium" });

		activity = activityStreams.getActivity({repo:this.repo});

		var newAct = activity[0];

		testlib.equal(itemTitle, newAct.title, "item title after modification");
		testlib.equal("Priority is now Medium.", newAct.what, "what after modification");
	};

	this.testShowRecentPushOfOldCommits = function()
	{
		var repoPath = repInfo.workingPath;
		var newRepoName = sg.gid();
		var clonePath = pathCombine(tempDir, newRepoName);

		// clone this.repo to repoclone
		this.exec("clone", ["vv", "clone", repInfo.repoName, newRepoName]);
		
		// create WD for repoclone
		if (! sg.fs.exists(clonePath))
			sg.fs.mkdir_recursive(clonePath);
		sg.fs.cd(clonePath);

		this.exec("checkout clone", ["vv", "checkout", newRepoName, "."]);
		sg.file.write("addedinclone.txt", "added in clone");
		vscript_test_wc__add("addedinclone.txt");
		csid = vscript_test_wc__commit( "added in clone");
		print("changeset checked in: " + csid);
		
		// back to this.repo and WD
		sg.fs.cd(repoPath);

		// add, commit 25x
		
		for ( var i = 1; i <= 30; ++i )
		{
			var fn = 'addedinmain' + i;
			sg.file.write(fn, "added in main");
			vscript_test_wc__add(fn);
			vscript_test_wc__commit( "added in main " + i);
		}

		sg.fs.cd(clonePath);

		this.exec("pull", ["vv", "pull", repInfo.repoName]);
		o = this.exec("heads", ["vv", "heads"]);

		var oursSeen = false;
		var other = null;

		var lines = o.stdout.split(/[\r\n]+/);

		var matches;
		for ( i = 0; i < lines.length; ++i )
		{
			matches = lines[i].match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);
			
			if (matches)
			{
				var cs = matches[1];

				if (cs == csid)
					oursSeen = true;
				else
				{
					if (other)
					{
						testlib.log(o.stdout);
						testlib.fail("expected only two heads");
						return;
					}

					other = matches[1];
				}
			}
		}

		this.exec("merge", ["vv", "merge", "-r", other]);
		
		vscript_test_wc__commit( "commit after merge");

		this.exec("push", ["vv", "push", repInfo.repoName]);

		sg.fs.cd(repoPath);

		var activity = activityStreams.getActivity({repo:this.repo});

		var spotted = false;

		for ( i = 0; i < activity.length; ++i )
		{
			var act = activity[i];
			if (act.action != 'commit')
				continue;

			matches = act.link.match(/recid=(.+)/);
			if (matches && (matches[1] == csid))
			{
				spotted = true;
				break;
			}
		}

		testlib.testResult(spotted, "csid " + csid + " expected in stream");
	};
	
	// V00213
	this.testNoCommentsForMissingChangesets = function()
	{
		var nonce = "tncfmc-" + sg.gid();

		var db = new zingdb(this.repo, sg.dagnum.VC_COMMENTS);
		var ztx = db.begin_tx();

		var rec = ztx.new_record('item');
		rec.text = nonce;
		rec.csid = sg.gid();
		
		var ct = ztx.commit();

		if (ct.errors)
			throw sg.to_json(ct);

		var activity = activityStreams.getActivity({repo:this.repo});

		for ( var i = 0; i < activity.length; ++i )
			if (activity[i].what == nonce)
			{
				testlib.fail("Comment seen for missing changeset: " + sg.to_json(activity[i]));
				break;
			}
	};

	// U5404
	this.failing_testIncompleteBranchLists = function()
	{
		var master = repInfo.workingPath;
		var featureBranch = "feature-" + sg.gid();
		var featureDir = pathCombine(tempDir, featureBranch);
		var feederBranch = "feeder-" + sg.gid();
		var feederDir = pathCombine(tempDir, feederBranch);

		// tie main dir to master
		sg.fs.cd(master);
		this.exec("tie main to master", ["vv", "branch", "attach", "master"]);
		this.exec("clean up", ["vv", "revert", "--all"]);
		this.exec("update main", ["vv", "update"]);

		// create, tie feature dir
		sg.fs.mkdir(featureDir);
		sg.fs.cd(featureDir);
		this.exec("checkout in feature", ["vv", "checkout", repInfo.repoName, "."]);
		this.exec("tie to feature", ["vv", "branch", "new", featureBranch]);

		// create, tie feeder dir
		sg.fs.mkdir(feederDir);
		sg.fs.cd(feederDir);
		this.exec("checkout in feeder", ["vv", "checkout", repInfo.repoName, "."]);
		this.exec("tie to feeder", ["vv", "branch", "new", feederBranch]);

		// add a file to master
		sg.fs.cd(master);
		var fn = sg.gid();
		sg.file.write(fn, "text");
		// commit, save csid
		vscript_test_wc__add(fn);
		var csid = vscript_test_wc__commit( "added in master");
		
		// merge that to feature
		sg.fs.cd(featureDir);

		fn = sg.gid();
		sg.file.write(fn, "text");
		// commit, save csid
		vscript_test_wc__add(fn);
		vscript_test_wc__commit( "added in feature");
		this.exec("merge to feature", ["vv", "merge", "-r", csid]);
		vscript_test_wc__commit( "merge");

		for ( var i = 0; i < 13; ++i )
		{
			//	add a file to feeder
			sg.fs.cd(feederDir);
			fn = sg.gid();
			sg.file.write(fn, fn);
			vscript_test_wc__add(fn);
			vscript_test_wc__commit( "commit in feeder " + i);

			//	merge to master
			sg.fs.cd(master);
			this.exec("merge to master " + i, ["vv", "merge", "-b", feederBranch]);
			vscript_test_wc__commit( "merge");
		}

		var activity = activityStreams.getActivity({repo:this.repo});

		var spotted = null;

		for ( i = 0; i < activity.length; ++i )
		{
			var act = activity[i];
			if (act.action != 'commit')
				continue;

			matches = act.link.match(/recid=(.+)/);
			if (matches && (matches[1] == csid))
			{
				spotted = act;
				break;
			}
		}

		if (! testlib.testResult(!! spotted, "csid " + csid + " expected in stream"))
		{
			return;
		}

		if (! testlib.ok(!! spotted.branches, "expected branches"))
			return;

		testlib.ok(spotted.branches.indexOf(featureBranch) >= 0, "expected feature branch in " + sg.to_json(spotted.branches));
		testlib.ok(spotted.branches.indexOf("master") >= 0, "expected master branch in " + sg.to_json(spotted.branches));
	};

	this.testEmptyPriorityIsEmpty = function()
	{
		var title = "testEmptyPriorityIsEmpty";
		var bugId = newRecord('item', this.repo, { 'title': title });

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var rec = db.get_record('item', bugId);
		var id = rec.id;

		this.modifyBug(bugId, { 'status': 'fixed', 'priority': '' });

		var request = {
			'repo': this.repo
		};

		var activity = bugSource.getActivity(request, 4);

		var ourGuy = null;

		for ( var i = 0; i < activity.length; ++i )
			if (activity[i].action == 'Fixed ' + id)
			{
				ourGuy = activity[i];
				break;
			}

		if (! testlib.ok(!! ourGuy, 'expected activity: Fixed ' + id))
			return;

		var what = ourGuy.what || '';

		testlib.ok(! what.match(/^Priority/), "Activity shouldn't mention priority");
	};

	this.testCombineCommentAndStatus = function()
	{
		var title = "testCombineCommentAndStatus";
		var bugId = newRecord('item', this.repo, { 'title': title, 'status': 'open' });

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;
		var id = null;

		try
		{
			ztx = db.begin_tx();

			var rec = ztx.open_record('item', bugId);
			rec.status = 'fixed';
			id = rec.id;

			var cmt = ztx.new_record('comment');
			cmt.text = 'added comment';
			cmt.who = this.userId;
			cmt.item = bugId;

			var c = ztx.commit();
			if (c.errors)
				throw sg.to_json(c);
			ztx = null;

			var request = {
				'repo': this.repo
			};

			var activity = activityStreams.getActivity({repo:this.repo});

			var last = activity[0];
			var previous = activity[1];

			if (! (testlib.equal('Fixed ' + id, last.action) &&
				testlib.equal('added comment', last.what) &&
				testlib.equal('Created ' + id, previous.action)))
			{
				testlib.log(sg.to_json__pretty_print([last, previous, activity[2]]));
			}
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

	this.testCombineCommentAndPriority = function()
	{
		var title = "testCombineCommentAndStatus";
		var bugId = newRecord('item', this.repo, { 'title': title, 'priority': 'Low' });

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;
		var id = null;

		try
		{
			ztx = db.begin_tx();

			var rec = ztx.open_record('item', bugId);
			rec.priority = 'High';
			id = rec.id;

			var cmt = ztx.new_record('comment');
			cmt.text = 'added comment';
			cmt.who = this.userId;
			cmt.item = bugId;

			var c = ztx.commit();
			if (c.errors)
				throw sg.to_json(c);
			ztx = null;

			var request = {
				'repo': this.repo
			};

			var activity = activityStreams.getActivity({repo:this.repo});

			var last = activity[0];
			var previous = activity[1];

			if (! (testlib.equal('Modified ' + id, last.action) &&
				testlib.equal("Priority is now High (was Low).\nComment: added comment", last.what) &&
				testlib.equal('Created ' + id, previous.action)))
			{
				testlib.log(sg.to_json__pretty_print([last, previous, activity[2]]));
			}
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

	this.testAssociatedCommitLink = function()
	{
		var repoPath = repInfo.workingPath;
		var newfn = sg.gid();
		var fn = pathCombine(repoPath, newfn);

		sg.file.write(fn, "hi");
		sg.fs.cd(repoPath);

		// clone this.repo to repoclone
		vscript_test_wc__add(newfn);
		var csid = vscript_test_wc__commit( "added new file");

		var inf = createNewRepo(sg.gid());
		var newRepoName = inf.repoName;

		var ztx = null;
		var r2 = sg.open_repo(newRepoName);

		var u = r2.create_user('yo');
		r2.close();
		r2 = sg.open_repo(newRepoName);

		try
		{
			var repoid = sg.list_descriptors()[repInfo.repoName].repo_id;
			var db = new zingdb(r2, sg.dagnum.WORK_ITEMS);

			ztx = db.begin_tx(null, u);

			var rec = ztx.new_record('item');
			rec.title = 'hi';
			var recid = rec.recid;

			vv.txcommit(ztx);
			ztx = null;

			ztx = db.begin_tx(null, u);
			rec = ztx.new_record('vc_changeset');
			rec.commit = repoid + ":" + csid;
			rec.item = recid;
			vv.txcommit(ztx);
			ztx = null;

			var activity = activityStreams.getActivity({'repo': r2});

			var act = activity[0];

			testlib.equal('/workitem.html?recid=' + recid, act.link, "link");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (r2)
				r2.close();
		}
	};

	this.testAssociatedCommitSameRepo = function()
	{
		var ztx = null;

		try
		{
			var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

			ztx = db.begin_tx();

			var rec = ztx.new_record('item');
			rec.title = 'testAssociatedCommitSameRepo';
			var recid = rec.recid;

			vv.txcommit(ztx);
			ztx = null;

			rec = db.get_record('item', recid);
			var id = rec.id;

			sg.fs.cd(repInfo.workingPath);

			var newfn = sg.gid();
			sg.file.write(newfn, sg.gid());

			vscript_test_wc__add(newfn);
			
			vscript_test_wc__commit( "added new file");
			
			newfn = sg.gid();
			sg.file.write(newfn, sg.gid());

			vscript_test_wc__add(newfn);
			o = this.exec("commit", ["vv", "commit", "-m", "added new file", "-a", repInfo.repoName + ":" + id]);

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

			if (! testlib.ok(!! matches, "no revision seen"))
				return;

			var activity = activityStreams.getActivity({'repo': this.repo});

			testlib.ok((activity[0].action == "commit") && (activity[1].action == "commit"), "don't note associated commits from the same repo");
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

    this.testActivityRepoEscape = function() {
		var repo = false;
		var ztx = false;

		var repName = sg.gid() + " foo";
		var escaped = encodeURIComponent(repName);

		var url = this.rootUrl + "/repos/" + escaped;
        var listUrl = url + "/activity.xml";

//        sg.vv.init_new_repo(['no-wc'], repName);
	sg.vv2.init_new_repo( { "repo" : repName, "no-wc" : true } );

		whoami_testing(repName);

		try
		{
			repo = sg.open_repo(repName);
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			ztx = db.begin_tx();
			var rec = ztx.new_record('item');
			rec.title = sg.gid();
			vv.txcommit(ztx);
			ztx = false;
		
			var request = {
				requestMethod: "GET",
				uri: "/repos/" + escaped,
				queryString: "",
				hostname: 'localhost',
				'repo': repo,
				'repoName': repName,
				'userid': null,
				'linkRoot': 'http://localhost'
			};

			var atom = activityStreams.getActivityAtom(request, null);
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}

		testlib.ok(! atom.match(new RegExp('link.+rel.+self.+' + repName)), 'no unescaped self link expected');
		testlib.ok(  atom.match(new RegExp('link.+rel.+self.+' + escaped)), 'escaped self link expected');
		testlib.ok(! atom.match(new RegExp('link.+rel.+alternate.+' + repName + '.+workitem')), 'no unescaped WI link expected');
		testlib.ok(  atom.match(new RegExp('link.+rel.+alternate.+' + escaped + '.+workitem')), 'escaped WI link expected');
    };

	this.testCombineCommentAndStatusSameBugOnly = function()
	{
		var title = "testCombineCommentAndStatusSameBugOnly";
		var bugId = newRecord('item', this.repo, { 'title': title, 'status': 'fixed' });
		var bugId2 = newRecord('item', this.repo, {'title': sg.gid(), 'status':'open'} );

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;
		var id = null;

		try
		{
			ztx = db.begin_tx();

			var rec = ztx.open_record('item', bugId);
			rec.status = 'verified';
			id = rec.id;

			var cmt = ztx.new_record('comment');
			cmt.text = 'added comment';
			cmt.who = this.userId;
			cmt.item = bugId2;

			var c = ztx.commit();
			if (c.errors)
				throw sg.to_json(c);
			ztx = null;

			var request = {
				'repo': this.repo
			};

			var activity = activityStreams.getActivity({repo:this.repo});

			var last = activity[0];
			var previous = activity[1];

			if (previous.action.match(/^Verified/) && ! (last.action.match(/^Verified/)))
			{
				previous = activity[0];
				last = activity[1];
			}

			if (! testlib.equal('Verified ' + id, last.action) &&
				testlib.notEqual('added comment', last.what))
			{
				testlib.log(sg.to_json__pretty_print([last, previous, activity[2]]));
			}
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};



	this.modifyBug = function(recid, newFields)
	{
		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;

		try
		{
			ztx = db.begin_tx();
			var rec = ztx.open_record('item', recid);

			for ( var f in newFields )
				rec[f] = newFields[f];

			vv.txcommit(ztx);
			ztx = null;
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

	this.exec = function(desc, args) 
	{
		var pdesc = desc;
		
		var o = sg.exec.apply(sg, args);

		if (! testlib.equal(0, o.exit_status, "exec for " + desc))
		{
			testlib.log(sg.to_json(args));
			testlib.log(o.stdout);
			testlib.log(o.stderr);
			throw("exec failed");
		}
			
		return(o);
	};
}
