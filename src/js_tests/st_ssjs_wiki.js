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

function st_ssjs_wiki()
{
	this.setUp = function()
	{
		this.repo = sg.open_repo(repInfo.repoName);

		var serverFiles = sg.local_settings().machine.server.files;
		var core = serverFiles + "/core/";
		var wiki = serverFiles + "/modules/wiki/";
		load(serverFiles + "/dispatch.js");
		load(core + "vv.js");
		load(core + "textCache.js");
		load(wiki + "wiki.js");
		load(core + "session.js");
		
		this.username1 = "wiki1@sourcegear.com";
		this.username2 = "wiki2@sourcegear.com";
		
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

	this.testConflictDescription = function()
	{
		var db = new zingdb(this.repo, sg.dagnum.WIKI);
		var ztx = null;

		try
		{
			ztx = db.begin_tx();
			var rec = ztx.new_record('page');
			var recid = rec.recid;
			rec.title = "testConflictDescription";
			rec.text = "line1\nline2\n";
			var ct = vv.txcommit(ztx);
			var csid = ct.csid;
			ztx = null;

			ztx = db.begin_tx(csid);
			rec = ztx.open_record('page', recid);
			rec.text = "line1\nline5\nline2\n";
			vv.txcommit(ztx);
			ztx = null;

			ztx = db.begin_tx(csid);
			rec = ztx.open_record('page', recid);
			rec.text = "line1\nline7\nline2\n";
			vv.txcommit(ztx);
			ztx = null;

			rec = db.get_record('page', recid);
			testlib.log("record after merge: " + sg.to_json__pretty_print(rec));

			var request = {
				'repo': this.repo,
				'pagename': "testConflictDescription"
			};

			var p = vvWikiModule.getPage(request);

			var ptext = p.text;

			testlib.ok(ptext.match(/^# Conflicting Edits Detected/), "conflict header expected");

			testlib.ok(! ptext.match(/ZING_MERGE_CONCAT/), "raw zing separator unexpected");
			testlib.ok(ptext.match(/# Conflicts with:/), "conflict separator expected");

			testlib.log(ptext);
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}
	};

	this.testConvertIncomingDescriptorsToIds = function()
	{
		var descriptor = repInfo.repoName;
		var repoId = this.repo.repo_id;

		var rec = {
			title: sg.gid(),
			text: "@" + descriptor + ":" + "g12345"
		};

		var request = {
			'repo': this.repo,
			'vvuser': this.userId
		};

		var recid = vvWikiModule.postWikiPage(request, rec);

		var db = new zingdb(this.repo, sg.dagnum.WIKI);

		var saved = db.get_record('page', recid);

		testlib.equal(rec.title, saved.title, 'title');
		testlib.equal("@" + repoId + ":" + "g12345", saved.text, "page text");
	};

	this.testCheckPage = function()
	{
		var descriptor = repInfo.repoName;
		var repoId = this.repo.repo_id;
		var goodTitle = sg.gid();
		var badTitle = sg.gid();

		var db = new zingdb(this.repo, sg.dagnum.WIKI);
		var ztx = db.begin_tx(null, this.userId);
		var newrec = ztx.new_record('page');
		newrec.title = goodTitle;
		newrec.text = "yes";
		vv.txcommit( ztx );

		var request = {
			'repo': this.repo,
			'pagename': goodTitle
		};

		testlib.ok(!! vvWikiModule.checkPage(request), "checkPage == true for existing page");

		request.pagename = badTitle;

		testlib.ok(!  vvWikiModule.checkPage(request), "checkPage == false for non-existing page");
	};

	this.testConvertOutgoingIdsToDescriptors = function()
	{
		var descriptor = repInfo.repoName;
		var repoId = this.repo.repo_id;

		var rec = {
			title: sg.gid(),
			text: "@" + repoId + ":" + "g12345"
		};

		var db = new zingdb(this.repo, sg.dagnum.WIKI);
		var ztx = db.begin_tx(null, this.userId);
		var newrec = ztx.new_record('page');
		newrec.title = rec.title;
		newrec.text = rec.text;
		vv.txcommit( ztx );

		var request = {
			'repo': this.repo,
			'pagename': rec.title,
			'vvuser': this.userId
		};

		var page = vvWikiModule.getPage(request);

		testlib.equal(rec.title, page.title, 'title');
		testlib.equal("@" + descriptor + ":" + "g12345", page.text, "page text");
	};

	this.testDuplicateTitle = function()
	{
		var title = sg.gid();

		var db = new zingdb(this.repo, sg.dagnum.WIKI);
		var ztx = db.begin_tx(null, this.userId);
		var newrec = ztx.new_record('page');
		newrec.title = title;
		newrec.text = 'first version';
		vv.txcommit( ztx );

		var request = {
			'repo': this.repo,
			'headers': {
				'From': this.userId
			}
		};

		var rec = {
			'title' : title,
			'text': 'second version'
		};
		
		var exMessage = false;
		var caught = false;

		try
		{
			var recid = vvWikiModule.postWikiPage(request, rec);
		}
		catch (e)
		{
			caught = true;
			exMessage = e.message;
		}

		if (testlib.ok(caught, "exception expected on duplicate title"))
			testlib.equal("duplicate title", exMessage, "exception text");
	};

	this.testRetrieveQuotesAndParens = function() 
	{
		var db = new zingdb(this.repo, sg.dagnum.WIKI);

		var qt = "\"title\"";
		var pt = "(parens)";

		var ztx = db.begin_tx(null, this.userId);
		var newrec = ztx.new_record('page');
		newrec.title = qt;
		newrec.text = qt;
		var qrec = newrec.recid;

		newrec = ztx.new_record('page');
		newrec.title = pt;
		newrec.text = pt;
		var prec = newrec.recid;

		vv.txcommit( ztx );

		var request = {
			'repoName': repInfo.repoName,
			'repo': this.repo,
			'pagename': qt
		};

		var page = vvWikiModule.getPage(request);

		testlib.ok(!! page, "expect page with quotes in name");

		request.pagename = pt;

		page = vvWikiModule.getPage(request);
		testlib.ok(!! page, "expect page with parens in name");
	};

	this.testForbidWikiCharsInTitles = function()
	{
		var repo = this.repo;
		var uid = this.userId;

		var shouldFail = function(title) {
			var rec = {
				'title': title,
				'text': "page titled " + title
			};

			var request = {
				'repo': repo,
				'vvuser': uid
			};
			
			var threw = false;

			try
			{
				var recid = vvWikiModule.postWikiPage(request, rec);
			}
			catch (e)
			{
				threw = true;
				testlib.ok(e.message.match(/Page titles must not contain/), "throw correct error");
			}
			
			testlib.ok(threw, "Page creation with " + title + " should fail");
		};
		
		shouldFail("left square [");
		shouldFail("right square ]");
		shouldFail("caret ^");
		shouldFail("at @");
		shouldFail("backslash \\");
	};

	this.testWikiDiff = function()
	{
		var repo = this.repo;
		var uid = this.userId;

		var pageName = sg.gid();
		
		var db = new zingdb(this.repo, sg.dagnum.WIKI);
		var ztx = null;

		try
		{
			ztx = db.begin_tx();
			var rec = ztx.new_record('page');
			var recid = rec.recid;
			rec.title = pageName;
			rec.text = "line1\nline2\n";
			var ct = vv.txcommit(ztx);
			var csid1 = ct.csid;
			ztx = null;

			ztx = db.begin_tx();
			rec = ztx.open_record('page', recid);
			rec.text = "line3\nline2\nline4\n";
			ct = vv.txcommit(ztx);
			var csid2 = ct.csid;
			ztx = null;
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}

		var rec = db.get_record('page', recid, ['*','history']);

		if (! testlib.ok(!! rec, "record"))
			return;

		var hid1, hid2;

		for ( var i = 0; i < rec.history.length; ++i )
			if (rec.history[i].csid == csid1)
				hid1 = rec.history[i].hidrec;
			else if (rec.history[i].csid == csid2)
				hid2 = rec.history[i].hidrec;

		if (! testlib.ok(hid1 && hid2, "hids"))
			return;

		var request = {
			'repo': this.repo,
			'vvuser': this.userId,
			'page': pageName,
			'csid1': csid1,
			'csid2': csid2
		};

 		var diff = vvWikiModule.getDiff(request);

		var diffLines = diff.split(/\r?\n/);

		testlib.ok(diffLines[0].match(/^---/));
		testlib.ok(diffLines[1].match(/^\+\+\+/));
		testlib.equal("@@ -1,2 +1,3 @@", diffLines[2]);
		testlib.equal("-line1", diffLines[3]);
		testlib.equal("+line3", diffLines[4]);
		testlib.equal(" line2", diffLines[5]);
		testlib.equal("+line4", diffLines[6]);
	};
}
