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

function st_ssjs_cache() {

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
		
		// create a sprint
		var ztx = null;
		
		try
		{
			var zs = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
			ztx = zs.begin_tx();
			var zrec = ztx.new_record("sprint");
			this.sprintId = zrec.recid;
			this.sprintName = "ssjscachesprint";
			zrec.description = "Common sprint for ssjs_cache tests.";
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

	this.newRecord = function(rectype, fields)
	{
		// create a sprint
		var ztx = null;
		var recid = null;
		
		try
		{
			var zs = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
			ztx = zs.begin_tx();
			var zrec = ztx.new_record(rectype);
			
			for ( var f in fields )
				zrec[f] = fields[f];

			recid = zrec.recid;

			ztx.commit();
			ztx = null;
		}
		catch(e)
		{
			recid = null;

			if (ztx)
				ztx.abort();
		}

		return(recid);
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

	this.testLinkItemToSprint = function()
	{
		var itemid = this.newRecord( "item", { title: "testLinkItemToSprint bug" } );

		var linkdata = {
			"recid" : this.sprintId
		};

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "milestone",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: itemid
		};

		var wir = new witRequest(request, workitemLinkFields);

		wir.updateLinks(request, linkdata);

		var rec = this.grabRecord('item', itemid);

		testlib.equal(this.sprintId, rec.milestone, "milestone after link");
	};

	this.testRemoveSprintLink = function()
	{
		var itemid = this.newRecord( "item", { title: "testLinkItemToSprint bug", milestone: this.sprintId } );

		var linkdata = { linklist: [] };

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "milestone",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: itemid
		};

		var wir = new witRequest(request, workitemLinkFields);

		wir.updateLinks(request, linkdata);

		var rec = this.grabRecord('item', itemid);

		testlib.testResult("undefined", typeof(rec.milestone), "milestone after unlink");
	};

	this.testBlockBug = function()
	{
		var original = this.newRecord( "item", { title: "original bug" } );
		var dup = this.newRecord("item", { title: "new bug" } );

		var linkdata = {
			linklist: [
				{ "recid" : original }
			]
		};

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "blockeditems",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: dup
		};

		var wir = new witRequest(request, workitemLinkFields);

		wir.updateLinks(request, linkdata);

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var w = vv.where(
			{
				relationship: 'blocks',
				target: original,
				source: dup
			}
		);

		var recs = db.query('relation', ['*'], w);

		testlib.equal(1, recs.length, "number of links");
	};

	this.testUnblockBug = function()
	{
		var original = this.newRecord( "item", { title: "original bug" } );
		var dup = this.newRecord("item", { title: "new bug" } );

		var linkdata = {
			linklist: [
				{ "recid" : original }
			]
		};

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "blockeditems",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: dup
		};

		var wir = new witRequest(request, workitemLinkFields);

		wir.updateLinks(request, linkdata);
		wir.updateLinks(request, { linklist: [] });

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var w = vv.where(
			{
				relationship: 'blocks',
				source: dup
			}
		);

		var recs = db.query('relation', ['*'], w);

		testlib.equal(0, recs.length, "number of links");
	};


	this.testBlockMultiple = function()
	{
		var original = this.newRecord( "item", { title: "original bug" } );
		var original2 = this.newRecord( "item", { title: "original 2" } );
		var dup = this.newRecord("item", { title: "new bug" } );

		var linkdata = {
			linklist: [
				{ "recid" : original },
				{ "recid" : original2 }
			]
		};

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "blockeditems",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: dup
		};

		var wir = new witRequest(request, workitemLinkFields);

		wir.updateLinks(request, linkdata);

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var w = vv.where(
			{
				relationship: 'blocks',
				source: dup
			}
		);

		var recs = db.query('relation', ['*'], w);

		if (! testlib.equal(2, recs.length, "number of links"))
			return;

		wir.updateLinks(request, { linklist: [ { "recid" : original2 } ] });
		recs = db.query('relation', ['*'], w);

		if (! testlib.equal(1, recs.length, "number of links after resetting to 1"))
			return;

		testlib.equal(original2, recs[0].target, "target");
	};


	this.testAddComment = function()
	{
		var itemid = this.newRecord( "item", { title: "testAddComment bug" } );

		var linkdata = {
			text: "new comment",
			who: this.userId
		};

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "comments",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: itemid
		};

		var wir = new witRequest(request, workitemLinkFields);

		wir.updateLinks(request, linkdata);

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var where = vv.where( 
			{
				"item": itemid
			}
		);

		var recs = db.query('comment', ['*'], where);

		testlib.equal(1, recs.length, "number of comments");
	};

	this.testDependOn = function()
	{
		var original = this.newRecord( "item", { title: "original bug" } );
		var blocker = this.newRecord("item", { title: "blocking bug" } );

		var linkdata = {
			linklist: [
				{ "recid" : blocker }
			]
		};

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "blockingitems",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: original
		};

		var wir = new witRequest(request, workitemLinkFields);

		sg.log("calling updateLinks");
		wir.updateLinks(request, linkdata);

		var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var w = vv.where(
			{
				relationship: 'blocks',
				target: original,
				source: blocker
			}
		);

		var recs = db.query('relation', ['*'], w);

		testlib.equal(1, recs.length, "number of links");
	};

	this.testLinkCS = function()
	{
        sg.file.write("testLinkCS.txt", "testLinkCS");
        vscript_test_wc__add("testLinkCS.txt");
        var changesetId = vscript_test_wc__commit();

		var bug = this.newRecord( "item", { title: "testLinkCS bug" } );

		var linkdata = {
			linklist: [
				{ "commit" : changesetId }
			]
		};

		var request = 
		{
			requestMethod: "POST",
			uri: "blah",
			linkname: "changesets",
			repoName: repInfo.repoName,
			repo: this.repo,
			queryString: "",
			headers: { From : this.userId },
			vvuser : this.userId ,

			recid: bug
		};

		var wir = new witRequest(request, workitemLinkFields);

		sg.log("calling updateLinks");
		wir.updateLinks(request, linkdata);

        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

		var recs = db.query('vc_changeset',
							['*'],
							vv.where(
								{
									commit: changesetId,
									item: bug
								}
							)
							);

        testlib.equal(1, recs.length, "changeset link count");
	};
}
