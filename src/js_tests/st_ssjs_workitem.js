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

function st_ssjs_workitem() {

	this.setUp = function()
	{
		this.repo = sg.open_repo(repInfo.repoName);
		var serverFiles = sg.local_settings().machine.server.files;
		var core = serverFiles + "/core";
		var scrum = serverFiles + "/modules/scrum";
		load(serverFiles + "/dispatch.js");
		load(core + "/vv.js");
		load(core + "/textCache.js");
		load(scrum + "/workitems.js");
		load(core + "/session.js");
		
		// create a sprint
		var ztx = null;
		
		try
		{
			var zs = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
			ztx = zs.begin_tx();
			var zrec = ztx.new_record("sprint");
			this.sprintId = zrec.recid;
			this.sprintName = "ssjsworkitemsprint";
			zrec.description = "Common sprint for ssjs_workitem tests.";
			zrec.name = this.sprintName;
			ztx.commit();
			ztx = false;

			var udb = new zingdb(this.repo, sg.dagnum.USERS);
			var urec = udb.query('user', ['*'], "name == '" + repInfo.userName + "'");

			this.userId = urec[0].recid;

			this.repo.set_user(repInfo.userName);
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};
	
	this.tearDown = function()
	{
		if (this.repo)
			this.repo.close();
	};

	this.newRecord = function(rectype, fields, repo, user)
	{
		// create a sprint
		var ztx = null;
		var recid = null;
		repo = repo || this.repo;
		user = user || null;
		
		try
		{
			var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			ztx = zs.begin_tx(null, user);
			var zrec = ztx.new_record(rectype);
			
			for ( var f in fields )
				zrec[f] = fields[f];

			recid = zrec.recid;

			vv.txcommit(ztx);
			ztx = null;

			return(recid);
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

	this.grabRecord = function(rectype, recid)
	{
		// create a sprint
		var zrec = null;
		
		try
		{
			var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
			zrec = db.get_record(rectype, recid);
		}
		catch(e)
		{
			zrec = null;
		}

		return(zrec);
	};

	this.testPartsInTheRightPlace = function()
	{
		var itemid = this.newRecord( "item", { title: "testPartsInTheRightPlace bug" } );
		var blocker = this.newRecord('item', { title: 'tpitrpBlocker', status: 'open' });
		var blockrel = this.newRecord('relation', 
									  {
										  'relationship': 'blocks',
										  'source': blocker,
										  'target': itemid
									  }
									 );
		var blocks = this.newRecord('item', { title: 'tpitrpBlocks' } );
		var blocksrel = this.newRecord('relation', { relationship: 'blocks', 'source': itemid, 'target': blocks } );
		var dup = this.newRecord('item', { title: 'tpitrpDup' } );
		var duprel = this.newRecord('relation', { relationship: 'duplicateof', 'source': itemid, 'target': dup } );

		var relto = this.newRecord('item', { title: 'tpitrpRelto' } );
		this.newRecord('relation', { relationship: 'related', 'source': itemid, 'target': relto } );
		var relfrom = this.newRecord('item', { title: 'tpitrpRelfrom' } );
		this.newRecord('relation', { relationship: 'related', 'target': itemid, 'source': relfrom } );
		var cmt = this.newRecord('comment', 
								 {
									 text: 'hi', who: this.userId, item: itemid
								 }
								);
		this.newRecord('work', { amount: 60, item: itemid } );

		var attfn = "tpitrp-attach.txt";
		sg.file.write(attfn, "hi");

		this.newRecord('attachment', 
					   {
						   contenttype: 'text/plain',
						   filename: attfn,
						   data: './' + attfn,
						   item: itemid
					   }
					  );

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		var data = wir.fullWorkItem();

		testlib.ok(data.id, 'id expected');
		testlib.ok(data.comments, 'comments expected');
		testlib.ok(data.blockers, 'blockers expected');
		testlib.ok(data.blocking, 'blocking expected');
		testlib.ok(data.duplicateof, 'duplicateof expected');
		if (testlib.ok(data.related, 'related expected'))
			testlib.equal(2, data.related.length, '2 relations');

		// all relations should be:
		// { other: recid, id: id, desc: desc }

		testlib.ok(data.blockers[0].other, 'expect other on blockers');
		testlib.ok(data.blocking[0].other, 'expect other on blocking');
		testlib.ok(data.duplicateof[0].other, 'expect other on duplicateof');
		testlib.ok(data.related[0].other, 'expect other on related');
		testlib.ok(data.related[1].other, 'expect other on related');

		testlib.ok(data.attachments, 'attachments expected');

		testlib.ok(data.milestones, 'milestones expected');
		testlib.ok(data.users, 'users expected');

		testlib.ok(data.work, 'work expected');

		testlib.ok(data._csid, '_csid expected');
	};

	this.testMirrorDuplicates = function()
	{
		var realone = this.newRecord( "item", { title: "testMirrorDuplicates main" } );
		var dupone = this.newRecord("item", { title: "testMirrorDuplicates dup 1" });
		var duptwo = this.newRecord("item", { title: "testMirrorDuplicates dup 2" });

		this.newRecord('relation',
					   {
						   'relationship': 'duplicateof',
						   'source': dupone,
						   'target': realone
					   }
					  );
		this.newRecord('relation',
					   {
						   'relationship': 'duplicateof',
						   'source': duptwo,
						   'target': realone
					   }
					  );

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: realone,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		var data = wir.fullWorkItem();

		testlib.equal(2, data.duplicates.length, 'dup count');

		var dupthree = this.newRecord('item',  { title: "testMirrorDuplicates dup 3"} );

		data.duplicates.push({
								 'other': dupthree
							 }
							);
		wir = new witRequest(request);
		wir.updateWorkItem(data);

		wir = new witRequest(request);
		data = wir.fullWorkItem();

		testlib.equal(3, data.duplicates.length, 'duplicate count');

		request.recid = dupthree;
		wir = new witRequest(request);
		data = wir.fullWorkItem();

		testlib.ok(!! data.duplicateof, 'any duplicate-of fields found') &&
			testlib.equal(realone, data.duplicateof[0].other, 'duplicate field after reverse setting');
	};


	this.testUpdateInFull = function()
	{
		var itemid = this.newRecord( "item", { title: "testUpdateInFull bug" } );
		var blocker = this.newRecord('item', { title: 'tuifBlocker', status: 'open' });
		var blockrel = this.newRecord('relation', 
									  {
										  'relationship': 'blocks',
										  'source': blocker,
										  'target': itemid
									  }
									 );
		var blocks = this.newRecord('item', { title: 'tuifBlocks' } );
		var blocksrel = this.newRecord('relation', { relationship: 'blocks', 'source': itemid, 'target': blocks } );
		var dup = this.newRecord('item', { title: 'tuifDup' } );
		var duprel = this.newRecord('relation', { relationship: 'duplicateof', 'source': itemid, 'target': dup } );
		var newBlocker = this.newRecord('item', { title: 'tuifNewBlocker'} );

		var relto = this.newRecord('item', { title: 'tuifRelto' } );
		this.newRecord('relation', { relationship: 'related', 'source': itemid, 'target': relto } );
		var relfrom = this.newRecord('item', { title: 'tuifRelfrom' } );
		this.newRecord('relation', { relationship: 'related', 'target': itemid, 'source': relfrom } );
		var cmt = this.newRecord('comment', 
								 {
									 text: 'hi', who: this.userId, item: itemid
								 }
								);
		this.newRecord('work', { amount: 60, item: itemid } );

		var ztx = null;

		try
		{
			var request = {
				repoName: repInfo.repoName,
				repo: this.repo,
				recid: itemid,
				headers: {
					From: this.userId
				},
				vvuser: this.userId

			};

			var wir = new witRequest(request);
			var rec = wir.fullWorkItem();

			delete rec.work;
			delete rec.users;
			delete rec.sprints;

			// set a field
			rec.listorder = 1;

			// add a comment
			rec.comment = 'hi again';

			// add a blocker
			rec.blocking.push( this.dupRecord( rec.blockers[0] ) );
			rec.blockers[0]._delete = true;
			rec.blockers.push( { other: newBlocker } );

			// move blocker to blocking
			// drop duplication

			request = {
				repoName: repInfo.repoName,
				repo: this.repo,
				recid: itemid,
				headers: {
					From: this.userId
				},
				vvuser: this.userId

			};

			wir = new witRequest(request);
			wir.updateWorkItem(rec);
			
			var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
			var newRec = db.get_record('item', itemid);

			testlib.equal(1, newRec.listorder, "list order after update");

			var comments = db.query('comment', ['*'], vv.where( { 'item': itemid }));
			testlib.equal(2, comments.length, 'comment count after update');

			var blockers = db.query('relation', ['*'],
									vv.where( { 'relationship': 'blocks', 'target' : itemid } )
								   );
			if (testlib.equal(1, blockers.length, 'new blocker count'))
				testlib.equal(newBlocker, blockers[0].source, 'blocker id');

			var blocking = db.query('relation', ['*'],
									vv.where( { 'relationship': 'blocks', 'source' : itemid } )
								   );
			if (testlib.equal(2, blocking.length, 'new blocking count'))
			{
				var found = {};

				found[ blocking[0].target ] = true;
				found[ blocking[1].target ] = true;

				testlib.ok(found[blocker], 'expected former blocker in blocking list');
				testlib.ok(found[blocks],  'expected original blocker still in blocks list');
			}
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

	this.testUpdateCommits = function()
	{
		var itemid = this.newRecord( "item", { title: "testUpdateCommits bug" } );
		
		// add commit, get its CSID
		var fn = "thingy.txt";
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);
        o = sg.exec("vv", "commit", "-m", "committed" /*, fn    -- W1976 */);
		if (! testlib.equal(0, o.exit_status, "add result"))
			return;

		var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid = matches[1];

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		var rec = wir.fullWorkItem();

		testlib.ok((! rec.commits) || (rec.commits.length == 0), "no commits yet");

		if (! rec.commits)
			rec.commits = [];
		rec.commits.push( { "commit": csid });

		// add csid to commits array
		// post successfully
		wir = new witRequest(request);
		wir.updateWorkItem(rec);

		// should find a vc_changeset record with itemid/csid
		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
		var where = "(commit == '" + csid + "') && (item == '" + itemid + "')";
		var recs = db.query('vc_changeset', ['*'], where);

		if (! testlib.ok(!! recs, "vc_changeset query results expected"))
			return;
		if (! testlib.equal(1, recs.length, "vc_changeset count"))
			return;

		wir = new witRequest(request);
		rec = wir.fullWorkItem();

		if (testlib.ok(!! rec.commits, "expecting commits after association"))
			if (testlib.equal(1, rec.commits.length, "commits length after association"))
				testlib.equal(csid, rec.commits[0].commit, "commit ID");
	};

	this.testAssociateOnCreate = function()
	{
		// add commit, get its CSID
		var fn = "thingy22.txt";
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);
        o = sg.exec("vv", "commit", "-m", "committed" /*, fn    -- W1976 */);
		if (! testlib.equal(0, o.exit_status, "add result"))
			return;

		var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid = matches[1];

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var newBug = {
			"id": "TAOCbug",
			"title": "testAssociateOnCreate",
			"commits": [
				{
					"commit" : csid
				}
			]
		};

		var wir = new witRequest(request);
		var recid = wir.createWorkItem(newBug);

		request.recid = recid;
		wir = new witRequest(request);
		var rec = wir.fullWorkItem();

		if (testlib.ok(!! rec.commits, "expecting commits after association"))
			if (testlib.equal(1, rec.commits.length, "commits length after association"))
				testlib.equal(csid, rec.commits[0].commit, "commit ID");
	};

	this.testCreateInFull = function()
	{
		var blocker = this.newRecord('item', { title: 'tcifBlocker', status: 'open' });
		var blocking = this.newRecord('item', { title: 'tcifBlocking', status: 'open' });
		var duplicate = this.newRecord('item', { title: 'tcifDuplicate', status: 'open' });
		var related = this.newRecord('item', { title: 'tcifRelated', status: 'open' });

		var rec = {
			title: "tcif bug",
			listorder: 1,
			blockers: [
				{ other: blocker }
			],
			blocking: [
				{ other: blocking }
			],
			duplicateof: [
				{ other: duplicate }
			],
			related: [
				{ other: related }
			],
			stamps: [
				{ name: "test@sourcegear.com" }
			],
			comment: "hi there"
		};

		request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		wir = new witRequest(request);
		var itemid = wir.createWorkItem(rec);

		if (! testlib.ok(!! itemid, "no item id"))
			return;
		if (! testlib.equal("string", typeof(itemid), "result type"))
			return;

		var ztx = null;

		try
		{
			var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
			var newRec = db.get_record('item', itemid);

			testlib.equal(1, newRec.listorder, "list order");

			var blockers = db.query('relation', ['*'],
									vv.where( { 'relationship': 'blocks', 'target' : itemid } )
								   );
			if (testlib.equal(1, blockers.length, 'new blocker count'))
				testlib.equal(blocker, blockers[0].source, 'blocker id');

			var blocks = db.query('relation', ['*'],
									vv.where( { 'relationship': 'blocks', 'source' : itemid } )
								   );
			if (testlib.equal(1, blocks.length, 'new blocking count'))
				testlib.equal(blocking, blocks[0].target, 'blocker id');

			var dups = db.query('relation', ['*'],
									vv.where( { 'relationship': 'duplicateof', 'source' : itemid } )
								   );
			if (testlib.equal(1, dups.length, 'new dup count'))
				testlib.equal(duplicate, dups[0].target, 'dup id');

			var rels = db.query('relation', ['*'],
									vv.where( { 'relationship': 'related', 'source' : itemid } )
								   );
			if (testlib.equal(1, rels.length, 'new related count'))
				testlib.equal(related, rels[0].target, 'related id');

			var comments = db.query('comment', ['*'], vv.where( { 'item': itemid } ));

			if (testlib.equal(1, comments.length, "comment count"))
				testlib.equal("hi there", comments[0].text, "comment text");
				
			var stamps = db.query('stamp', ['*'], vv.where( { 'item': itemid } ));

			if (testlib.equal(1, stamps.length, "stamps count"))
				testlib.equal("test@sourcegear.com", stamps[0].name, "stamp name");
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

	this.testSkipDeletedOnCreate = function()
	{
		var blocker = this.newRecord('item', { title: 'tsdocBlocker', status: 'open' });
		var nonblocker = this.newRecord('item', { title: 'tsdocnonBlocker', status: 'open' });

		var rec = {
			title: "tcif bug",
			listorder: 1,
			blockers: [
				{ other: blocker },
				{ other: nonblocker, _delete: true }
			]
		};

		request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		wir = new witRequest(request);
		var itemid = wir.createWorkItem(rec);

		if (! testlib.ok(!! itemid, "no item id"))
			return;
		if (! testlib.equal("string", typeof(itemid), "result type"))
			return;

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var blockers = db.query('relation', ['*'],
								vv.where( { 'relationship': 'blocks', 'target' : itemid } )
							   );
		if (testlib.equal(1, blockers.length, 'new blocker count'))
			testlib.equal(blocker, blockers[0].source, 'blocker id');
	};

	this.testCreateNoWhoami = function()
	{
		var repo = false;
		var reponame = sg.gid();

		try
		{
			sg.vv2.init_new_repo({"repo":reponame, "no-wc":true});
			repo = sg.open_repo(reponame);
			var uname = sg.gid();
			var uid = repo.create_user(uname);
			repo.close();
			repo = sg.open_repo(reponame);

			var rec = {
				title: "tcif bug",
				listorder: 1,
				blockers: []
			};

			request = {
				repoName: reponame,
				'repo': repo,
				headers: {
					From: uid
				},
				vvuser: uid

			};

			wir = new witRequest(request);
			var itemid = wir.createWorkItem(rec);

			if (! testlib.ok(!! itemid, "no item id"))
				return;
			if (! testlib.equal("string", typeof(itemid), "result type expected"))
				return;
		}
		finally
		{
			if (repo)
				repo.close();
		}
	};


	this.testSkipDeletedCommitsOnCreate = function()
	{
		var there = sg.gid();
		var deleted = sg.gid();

		var rec = {
			title: "tcif bug",
			listorder: 1,
			commits: [
				{
					"commit": deleted,
					_delete: true
				},
				{
					"commit": there
				}
			]
		};

		request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		wir = new witRequest(request);
		var itemid = wir.createWorkItem(rec);

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var assoc = db.query('vc_changeset', ['*'], vv.where( { 'item': itemid } ));

		if (testlib.equal(1, assoc.length, 'association count'))
			testlib.equal(there, assoc[0].commit, 'association ID');
	};

	this.testDeleteSpecifiedRelations = function()
	{
		var repo = false;
		var reponame = sg.gid();

		try
		{
			sg.vv2.init_new_repo({"repo":reponame, "no-wc":true});
			repo = sg.open_repo(reponame);
			var uname = sg.gid();
			var uid = repo.create_user(uname);
			repo.close();
			repo = sg.open_repo(reponame);

			// create a record, pull it and grab current csid
			var itemid = this.newRecord('item', { title: 'testDSR' }, repo, uid);
			var otherrec = this.newRecord('item', { title: 'second cousin' }, repo, uid);
			var thirdrec = this.newRecord('item', { title: 'third one' }, repo, uid);

			var request = {
				repoName: reponame,
				'repo': repo,
				recid: itemid,
				headers: {
					From: uid
				},
				vvuser: uid

			};

			var wir = new witRequest(request);
			var data = wir.fullWorkItem();
			var saveCsid = data._csid;

			// create a new relation out-of-band
			this.newRecord(
				'relation',
				{
					relationship: 'related',
					source: itemid,
					target: otherrec
				},
				repo, uid
			);
			wir = new witRequest(request);
			var tdata = wir.fullWorkItem();

			testlib.ok(!! tdata.related, 'expect related items');
			testlib.equal(1, tdata.related.length, 'related count');

			// update the record as-is, should be safe
			wir = new witRequest(request);
			wir.updateWorkItem(data);

			// pull again, expect no relations
			wir = new witRequest(request);
			tdata = wir.fullWorkItem();
			var oneData = tdata;

			testlib.ok(tdata.related && (tdata.related.length == 1), "relations expected");

			this.newRecord(
				'relation',
				{
					relationship: 'related',
					source: itemid,
					target: thirdrec
				},
				repo, uid
			);

			wir = new witRequest(request);
			tdata = wir.fullWorkItem();

			if ((! testlib.ok(!! tdata.related, "relations member expected")) ||
				(! testlib.equal(2, tdata.related.length, "2 relations expected")))
				vv.log("tdata:", tdata);

			// kill the first relation
			oneData.related[0]._delete = true;
			wir = new witRequest(request);
			wir.updateWorkItem(oneData);

			wir = new witRequest(request);
			tdata = wir.fullWorkItem();

			if ((! testlib.ok(!! tdata.related, "relations member expected")) ||
				(! testlib.equal(1, tdata.related.length, "relations after delete")))
				vv.log("tdata:", tdata);

			testlib.equal(thirdrec, tdata.related[0].other, "remaining relation ID");
		}
		finally
		{
			if (repo)
				repo.close();
		}
	};

	this.testSearchForChangesets = function() {
		var u1 = "testing1@example.com";
		var gu1 = this.repo.create_user(u1);
		var u2 = "testing2@example.com";
		var gu2 = this.repo.create_user(u2);
		var repo = this.repo;

		var commitAs = function(userid, fn, cmt) {
			sg.file.write(fn, fn);
		vscript_test_wc__add(fn);
			sg.vv2.whoami({ "username" : userid });
			
			o = sg.exec("vv", "commit", "-m", cmt /*, fn     -- W1976 */);
			if (o.exit_status != 0)
				throw("unable to commit " + fn);

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);
			
			if (! matches)
				throw("no revision seen");

			var csid = matches[1];

			return(csid);
		};
		
		var csid_1_1 = commitAs(u1, "u1first", "u1 first");
		var csid_1_2 = commitAs(u1, "u1second", "u1 second");
		var csid_2_1 = commitAs(u2, "u2first", "u2 first");
		var csid_2_2 = commitAs(u2, "u2second", "u2 second");

		var request = {
			repoName: repInfo.repoName,
			'repo': repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var searchFor = function(text, user) {
			var qs = 'q=' + text;

			if (user)
				qs += '&userid=' + user;

			request.queryString = qs;

			var wir = new witRequest(request);
			var recs = wir.changesetMatch();

			var matched = {};

			for ( var i = 0; i < recs.length; ++i )
				matched[recs[i].value] = recs[i];

			return(matched);
		};

		var matches = searchFor("second");
		testlib.ok(!  matches[csid_1_1], "unexpected u1 first on second search");
		testlib.ok(!  matches[csid_2_1], "unexpected u2 first on second search");
		testlib.ok(!! matches[csid_1_2], "expected u1 second on second search");
		testlib.ok(!! matches[csid_2_2], "expected u2 second on second search");

		var label = matches[csid_1_2].label;

		matches = label.match(/^([0-9]+):([a-z0-9]+)…[ \t]+(.*)/);

		if (testlib.ok(!! matches, "unexpected label format: " + label))
		{
			var lcsid = matches[2];
			var ltext = matches[3];

			testlib.equal("u1 second", ltext, "label text");
			testlib.equal(csid_1_2.substring(0,10), lcsid, "csid text");
		}
		
		// filter on one user
		matches = searchFor("second", gu1);
		testlib.ok(!  matches[csid_1_1], "unexpected u1 first on second search");
		testlib.ok(!  matches[csid_2_1], "unexpected u2 first on second search");
		testlib.ok(!! matches[csid_1_2], "expected u1 second on second search");
		testlib.ok(!  matches[csid_2_2], "unexpected u2 second on second search");

		// substring matches
		matches = searchFor("firs");
		testlib.ok(!! matches[csid_1_1], "expected u1 first on second search");
		testlib.ok(!! matches[csid_2_1], "expected u2 first on second search");
		testlib.ok(!  matches[csid_1_2], "unexpected u1 second on second search");
		testlib.ok(!  matches[csid_2_2], "unexpected u2 second on second search");
	};

	this.testDeleteCommitAssociation = function() 
	{
		var itemid = this.newRecord( "item", { title: "testDeleteCommitAssociation bug" } );
		
		// add commit, get its CSID
		var fn = "thingy2.txt";
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);
        o = sg.exec("vv", "commit", "-m", "committed" /*, fn     -- W1976 */);
		if (! testlib.equal(0, o.exit_status, "add result"))
			return;

		var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid = matches[1];

		// add commit, get its CSID
		fn = "thingy3.txt";
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);
        o = sg.exec("vv", "commit", "-m", "committed" /*, fn    -- W1976 */);
		if (! testlib.equal(0, o.exit_status, "commit result"))
			return;

		matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid2 = matches[1];

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		var rec = wir.fullWorkItem();

		testlib.ok((! rec.commits) || (rec.commits.length == 0), "no commits yet");

		if (! rec.commits)
			rec.commits = [];
		rec.commits.push( {commit: csid } );
		rec.commits.push( {commit: csid2 });

		// add csid to commits array
		// post successfully
		wir = new witRequest(request);
		wir.updateWorkItem(rec);

		// should find a vc_changeset record with itemid/csid
		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
		var where = "item == '" + itemid + "'";
		var recs = db.query('vc_changeset', ['*'], where);

		if (! testlib.ok(!! recs, "vc_changeset query results expected"))
			return;
		if (! testlib.equal(2, recs.length, "vc_changeset count"))
			return;

		wir = new witRequest(request);
		rec = wir.fullWorkItem();

		var first = rec.commits[0].commit;

		rec.commits[1]._delete = true;

		wir = new witRequest(request);
		wir.updateWorkItem(rec);

		recs = db.query('vc_changeset', ['*'], where);

		if (! testlib.ok(!! recs, "vc_changeset query results expected"))
			return;
		if (! testlib.equal(1, recs.length, "vc_changeset count after delete"))
			return;

		testlib.equal(first, recs[0].commit, "remaining csid");
	};

	this.testSearchForChangesetIncludesHid = function() {
		var u1 = "tsfcih@example.com";
		this.repo.create_user(u1);

		var repo = this.repo;

		var commitAs = function(userid, fn, cmt) {
			sg.file.write(fn, fn);
		vscript_test_wc__add(fn);
			sg.vv2.whoami({ "username" : userid });
				
			o = sg.exec("vv", "commit", "-m", cmt /*, fn    -- W1976 */);
			if (o.exit_status != 0)
				throw("unable to commit " + fn);

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);
			
			if (! matches)
				throw("no revision seen");

			var csid = matches[1];

			return(csid);
		};
		
		// oldest is a higher-weighted FTS match
		var csid_1_1 = commitAs(u1, "u1csfirst2", "text text text");
		sleep(2000);
		var csid_1_2 = commitAs(u1, "u1cssecond2", "texting");

		var request = {
			repoName: repInfo.repoName,
			'repo': repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var searchFor = function(text, user) {
			var qs = 'q=' + text;

			if (user)
				qs += '&userid=' + user;

			request.queryString = qs;

			var wir = new witRequest(request);
			var recs = wir.changesetMatch();

			return(recs);
		};

		var matches = searchFor(csid_1_1);

		if (testlib.equal(1, matches.length, "match count"))
			testlib.equal(csid_1_1, matches[0].value, "match csid");
	};

	this.testSearchForChangesetIncludesRevId = function() {
		var u1 = "tsfcir@example.com";
		this.repo.create_user(u1);

		var repo = this.repo;

		var commitAs = function(userid, fn, cmt) {
			sg.file.write(fn, fn);
		vscript_test_wc__add(fn);
			sg.vv2.whoami({ "username" : userid });
				
			o = sg.exec("vv", "commit", "-m", cmt /*, fn    -- W1976 */);
			if (o.exit_status != 0)
				throw("unable to commit " + fn);

			var matches = o.stdout.match(/revision:[ \t]+([0-9]+):([0-9a-f]+)/);
			
			if (! matches)
				throw("no revision seen");

			return( [ matches[1], matches[2] ] );
		};
		
		// oldest is a higher-weighted FTS match
		var details = commitAs(u1, sg.gid(), "text text text");
		var revid = details[0];
		var csid = details[1];

		var request = {
			repoName: repInfo.repoName,
			'repo': repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var searchFor = function(text, user) {
			var qs = 'q=' + text;

			if (user)
				qs += '&userid=' + user;

			request.queryString = qs;

			var wir = new witRequest(request);
			var recs = wir.changesetMatch();

			return(recs);
		};

		var matches = searchFor(revid);

		if (testlib.equal(1, matches.length, "match count"))
			testlib.equal(csid, matches[0].value, "match csid");
	};

	this.testSearchForChangesetOnlyHidOrRecid = function() {
		var u1 = "tsfcohor@example.com";
		this.repo.create_user(u1);

		var repo = this.repo;

		var commitAs = function(userid, fn, cmt) {
			sg.file.write(fn, fn);
		vscript_test_wc__add(fn);
			sg.vv2.whoami({ "username" : userid });
			
			o = sg.exec("vv", "commit", "-m", cmt /*, fn    -- W1976 */);
			if (o.exit_status != 0)
				throw("unable to commit " + fn);

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);
			
			if (! matches)
				throw("no revision seen");

			var csid = matches[1];

			return(csid);
		};
		
		// oldest is a higher-weighted FTS match
		var hidId = commitAs(u1, "hidIdCommit", "blah2");
		var revnoId = commitAs(u1, "csIdCommit", "blah");

		var desc = repo.get_changeset_description(revnoId);
		var revno = desc.revno;

		commitAs(u1, sg.gid(), "this has the revno: " + revno + " ");
		commitAs(u1, sg.gid(), "this has the hid: " + hidId + " ");

		var request = {
			repoName: repInfo.repoName,
			'repo': repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var searchFor = function(text) {
			var qs = 'q=' + text;

			request.queryString = qs;

			var wir = new witRequest(request);
			var recs = wir.changesetMatch();

			return(recs);
		};

		var matches = searchFor(hidId);

		if (testlib.equal(1, matches.length, "match count on hid"))
			testlib.equal(hidId, matches[0].value, "match hid");

		matches = searchFor(revno);

		if (testlib.equal(1, matches.length, "match count on revno"))
			testlib.equal(revnoId, matches[0].value, "match hid");
	};


	this.testSearchForChangesetExactIgnoresFilter = function() {
		var u1 = sg.gid();
		this.repo.create_user(u1);
		var u2 = sg.gid();
		this.repo.create_user(u2);

		var repo = this.repo;

		var commitAs = function(userid, fn, cmt) {
			sg.file.write(fn, fn);
		vscript_test_wc__add(fn);
			sg.vv2.whoami({ "username" : userid });
				
			o = sg.exec("vv", "commit", "-m", cmt /*, fn    -- W1976 */);
			if (o.exit_status != 0)
				throw("unable to commit " + fn);

			var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);
			
			if (! matches)
				throw("no revision seen");

			var csid = matches[1];

			return(csid);
		};
		
		var revnoId = commitAs(u1, sg.gid(), "blah");

		var desc = repo.get_changeset_description(revnoId);
		var revno = desc.revno;

		var request = {
			repoName: repInfo.repoName,
			'repo': repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var searchFor = function(text, user) {
			var qs = 'q=' + text;

			if (user)
				qs += "&userid=" + user;

			request.queryString = qs;

			var wir = new witRequest(request);
			var recs = wir.changesetMatch();

			return(recs);
		};

		var matches = searchFor(revno, u1);

		if (testlib.equal(1, matches.length, "match count on committing user"))
			testlib.equal(revnoId, matches[0].value, "match hid");

		matches = searchFor(revno, u2);

		if (testlib.equal(1, matches.length, "match count on non-committing user"))
			testlib.equal(revnoId, matches[0].value, "match hid");
	};


	this.testDeleteCommitAPI = function() 
	{
		var itemid = this.newRecord( "item", { title: "testDeleteCommitAPI bug" } );
		
		// add commit, get its CSID
		var fn = sg.gid();
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);
        o = sg.exec("vv", "commit", "-m", "committed", /* fn,  -- W1976 */ "-a", itemid);
		if (! testlib.equal(0, o.exit_status, "commit result"))
			return;

		var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid = matches[1];

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		var data = wir.fullWorkItem();

		testlib.equal(1, data.commits.length, "commit length before delete");

		request.csid = csid;

		wir = new witRequest(request);
		wir.deleteCommit();

		wir = new witRequest(request);
		data = wir.fullWorkItem();

		testlib.equal(0, data.commits.length, "commit length after delete");
		
	};

	this.testAddCommitAPI = function() 
	{
		var itemid = this.newRecord( "item", { title: "testAddCommitAPI bug" } );
		
		// add commit, get its CSID
		var fn = sg.gid();
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);
        o = sg.exec("vv", "commit", "-m", "committed" /*, fn   -- W1976 */);
		if (! testlib.equal(0, o.exit_status, "commit result"))
			return;

		var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid = matches[1];

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		var data = wir.fullWorkItem();

		testlib.equal(0, data.commits.length, "commit length before add");

		request.csid = csid;

		wir = new witRequest(request);
		wir.addCommit();

		wir = new witRequest(request);
		data = wir.fullWorkItem();

		testlib.equal(1, data.commits.length, "commit length after add");
		
	};


	this.testWitHistoryRelations = function() 
	{
		var itemid = this.newRecord( "item", { title: "testWitHistoryRelations bug" } );
		var rel1 = this.newRecord('item', {
									   title: sg.gid()
								  }
								 );
		var rel2 = this.newRecord('item', {
									   title: sg.gid()
								  }
								 );

		var ztx = null;
		var recid = null;
		
		try
		{
			var zs = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
			ztx = zs.begin_tx();

			var r1 = ztx.new_record('relation');
			r1.relationship = "related";
			r1.source = itemid;
			r1.target = rel1;
			var r2 = ztx.new_record('relation');
			r2.relationship = 'related';
			r2.source = rel2;
			r2.target = itemid;

			vv.txcommit(ztx);
			ztx = null;
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
		
		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var hist = new vvWitHistory(request, request.recid);

		var results = hist.getHistory();

		testlib.equal(2, results.length, "result count");

		var res = results[0];
		testlib.ok(!! res['Related To'], "related to seen");

		var rels = res['Related To'];
		testlib.equal(2, rels.length, 'related-to count');
	};


	this.testBatchUpdate = function()
	{
		var rec1 = this.newRecord('item', { 'title': 'rec1', 'status': 'open' } );
		var rec2 = this.newRecord('item', { 'title': 'rec2', 'status': 'open' } );
		var rec3 = this.newRecord('item', { 'title': 'rec3', 'status': 'open' } );
		var rec4 = this.newRecord('item', { 'title': 'rec4', 'status': 'open' } );
		var repo = this.repo;
		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

		var getCsid = function(recid)
		{
			var rec = db.get_record('item', recid, ['recid','history']);
			var hist = vv.getLastHistory(rec);
            print(sg.to_json__pretty_print(hist));
			return(hist.csid);
		};

		try
		{
			var	ztx = db.begin_tx();
			var r1 = ztx.open_record('item', rec1);
			var r2 = ztx.open_record('item', rec2);
			r1.title = 'rec1 modified';
			r2.title = 'rec2 modified';
			vv.txcommit(ztx);
			ztx = false;
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}

		var mods = [];

		var recids = [rec1, rec2, rec3, rec4];
		for ( var i = 0; i < recids.length; ++i )
		{
			var rid = recids[i];
			var csid = getCsid(rid);

			mods.push ( {
				'recid': rid,
				'status': 'verified',
				'_csid': csid
			} );
		}

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		// send the mods over
		var wir = new witRequest(request);
		var recs = wir.batchUpdateItems(mods);
		var r = {};

		if (! testlib.equal(4, recs.length, "record count"))
			return;

		// expect a list of full records (check for title)
		for ( i = 0; i < recs.length; ++i )
		{
			var rec = recs[i];
			testlib.ok(!! rec.recid, "recid expected");
			testlib.equal('verified', rec.status, "status");
			testlib.ok(!! rec.id, "id seen");
			testlib.ok(!! rec._csid, "_csid seen");

			r[rec.recid] = rec;
		}

		print(sg.to_json__pretty_print(r));

        print("rec1 ", rec1);
        print("rec2 ", rec2);
        print("rec3 ", rec3);
        print("rec4 ", rec4);

		testlib.ok(r[rec1]._csid == r[rec2]._csid, "recs with same initial csid should have matching final csid");
		testlib.ok(r[rec1]._csid != r[rec3]._csid, "rec1 & rec3 csid should not match");
		testlib.ok(r[rec1]._csid != r[rec4]._csid, "rec1 & rec4 csid should not match");
		testlib.ok(r[rec3]._csid != r[rec4]._csid, "rec3 & rec4 csid should not match");
	};

	this.testDiscardDuplicateRelations = function()
	{
		// create a record, pull it and grab current csid
		var otherrec = this.newRecord('item', { title: 'ka-block' });
		var itemid = this.newRecord('item', { title: 'testDDR' });

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		var data = wir.fullWorkItem();

		testlib.equal(0, data.blockers.length, "blocker count before update");

		data.blockers = [
			{ 'other': otherrec },
			{ 'other': otherrec }
		];

		wir = new witRequest(request);
		wir.updateWorkItem(data);

		wir = new witRequest(request);
		data = wir.fullWorkItem();

		testlib.equal(1, data.blockers.length, "blocker count");
	};

	this.testAvoidRedundantCommits = function()
	{
		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var itemid = this.newRecord( "item", { title: "testAvoidRedundantCommits bug" } );
		var rec = db.get_record('item', itemid);
		var id = rec.id;
		
		// add commit, get its CSID
		var fn = sg.gid();
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);

        o = sg.exec("vv", "commit", "-m", "committed", "-a", repInfo.repoName + ":" + id);
		if (! testlib.equal(0, o.exit_status, "commit result"))
			return;

		var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid = matches[1];

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId

		};

		var wir = new witRequest(request);
		rec = wir.fullWorkItem();

		if (! testlib.ok(rec.commits && (rec.commits.length == 1), "one commit before update"))
			return;

		rec.title = rec.title + " after";

		// add csid to commits array
		// post successfully
		wir = new witRequest(request);
		wir.updateWorkItem(rec);

		var where = "item == '" + itemid + "'";
		var recs = db.query('vc_changeset', ['*'], where);

		if (! testlib.ok(!! recs, "vc_changeset query results expected"))
			return;
		if (! testlib.equal(1, recs.length, "vc_changeset count"))
			return;
	};

	this.testCleanFailOnBadWitSearch = function()
	{
		var req = {
			queryString: 'term=SOMETHINGELSE)',
			repo: this.repo
		};

		var wir = new witRequest(req);
		
		var results = wir.search(['id','title']);

		testlib.equal(0, results.length, "no results expected");
	};

	this.testExcludeRedundantRepoFromCsLink = function()
	{
		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			headers: {
				From: this.userId
			},
			vvuser: this.userId
		};

		var newBug = {
			"title": "testExcludeRedundantRepoFromCsLink"
		};

		var wir = new witRequest(request);
		var recid = wir.createWorkItem(newBug);

		request.recid = recid;
		wir = new witRequest(request);
		var rec = wir.fullWorkItem();

		var bugid = rec.id;
		var bugstr = repInfo.repoName + ":" + bugid;

		// add commit, get its CSID
		var fn = sg.gid();
		sg.file.write(fn, "thingy");

		vscript_test_wc__add(fn);
        o = sg.exec("vv", "commit", "-m", "committed", "-a", bugstr);
		if (! testlib.equal(0, o.exit_status, "commit result"))
		{
			testlib.log(bugstr);
			testlib.log(o.stdout);
			testlib.log(o.stderr);
			return;
		}

		var matches = o.stdout.match(/revision:[ \t]+[0-9]+:([0-9a-f]+)/);

		if (! testlib.ok(!! matches, "no revision seen"))
			return;

		var csid = matches[1];

		wir = new witRequest(request);
		rec = wir.fullWorkItem();

		if (testlib.ok(!! rec.commits, "expecting commits after association"))
			if (testlib.equal(1, rec.commits.length, "commits length after association"))
				testlib.equal("- committed", rec.commits[0].title, "title");
	};

	this.testSelfToSelfRelOnce = function()
	{
		var itemid = this.newRecord( "item", { title: "testSelfToSelfRelOnce bug" } );
		this.newRecord('relation', { relationship: 'related', 'source': itemid, 'target': itemid } );

		var request = {
			repoName: repInfo.repoName,
			repo: this.repo,
			recid: itemid,
			headers: {
				From: this.userId
			},
			vvuser: this.userId
		};

		var wir = new witRequest(request);
		var data = wir.fullWorkItem();

		if (! testlib.equal(1, data.related.length, "related count"))
			vv.log("full", data.related);
	};

	this.dupRecord = function(rec)
	{
		var result = {};

		for ( var fn in rec )
			result[fn] = rec[fn];

		return(result);
	};

}
