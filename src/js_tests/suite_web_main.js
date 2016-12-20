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

function suite_web_main(hasWorkingCopy) {

    this.serverHasWorkingCopy = hasWorkingCopy;
    this.suite_setUp = function() {
		{
			var repo = sg.open_repo(repInfo.repoName);

			{
				var zs = new zingdb(repo, sg.dagnum.USERS);
				var urec = zs.query('user', ['*'], "name == '" + repInfo.userName + "'");

				this.userId = urec[0].recid;
			}

			repo.set_user(repInfo.userName);

			repo.close();
		}

        // Create a changeset for future reference
        {
            sg.file.write("stWeb.txt", "stWeb");
            vscript_test_wc__add("stWeb.txt");
            this.changesetId = vscript_test_wc__commit();

            sg.file.write("stWeb2.txt", "stWeb2");
            vscript_test_wc__add("stWeb2.txt");
            this.changeset2Id = vscript_test_wc__commit();

            sg.file.write("stWeb3.txt", "stWeb3");
            vscript_test_wc__add("stWeb3.txt");
            this.changeset3Id = vscript_test_wc__commit();
        }

        // Set up our zing databases.
        {
            var repo = sg.open_repo(repInfo.repoName);

            {
                var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
                var ztx = zs.begin_tx();
                var zrec = ztx.new_record("item");
                this.bugId = zrec.recid; // If you modify the bug, please leave the string "Common" in the description.
                this.bugSimpleId = "123456";
                zrec.title = "Common bug for stWeb tests.";
                zrec.id = this.bugSimpleId;
                ztx.commit();

                // and a sprint

                ztx = zs.begin_tx();
                zrec = ztx.new_record("sprint");
                this.sprintId = zrec.recid; // If you modify the bug, please leave the string "Common" in the description.
                this.sprintName = "dogfood";
                zrec.description = "Common sprint for stWeb tests.";
                zrec.name = this.sprintName;
                ztx.commit();

                // and a config record
                ztx = zs.begin_tx();
                zrec = ztx.new_record("config");
                this.configId = zrec.recid;
                this.configKey = "testKey";
                zrec.key = this.configKey;
                zrec.value = "testValue";
                ztx.commit();

            }

            repo.close();
        }

    };

	this.testVersion = function() {
		testlib.log(sg.version);
		var o = curl(this.rootUrl+'/version.txt');
		testlib.testResult(o.body == sg.version);
	};

    this._addSearchable = function(title, findable) {
        var repo = sg.open_repo(repInfo.repoName);
        var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

	var  ztx = zs.begin_tx();
	var bugid = title.replace(/ +/g, '');

	var zrec = ztx.new_record("item");
	this.searchables[zrec.recid] = findable;
	zrec.title = title;
	zrec.id = bugid;

	ztx.commit();

	repo.close();
    };


    this.checkStatus = function(expected, actual, msg, stdout) {
        var prefix = "";

        if (!!msg)
            prefix = msg + ": ";

        if (actual == expected)
            return testlib.testResult(true, "===== "+expected+" =====");

        var err = prefix + "expected '" + expected + "', got '" + actual + "'";

		if (stdout)
			testlib.log(stdout);

        return (testlib.fail(err));
    };


    this.verify405_postdata = function(path, addJsonHeader, postdata) {
        var url = this.rootUrl + path;
        var o;

        if (addJsonHeader) {
            o = curl("-H", "Accept: application/json", "-d", postdata, url);
        }
        else
          o = curl("-d", postdata, url);

        return (this.checkStatus("405 Method Not Allowed", o.status, url));
    };

    this.verify404 = function(path) {
        var url = this.rootUrl + path;
        var o;
        o = curl(url);

        return (this.checkStatus("404 Not Found", o.status, url, o.body));
    };

    this.verify301 = function(path) {
        var url = this.rootUrl + path;
        var o = curl(url);

        return (this.checkStatus("301 Moved Permanently", o.status, url, o.body));
    };

    this.verifyNot404 = function(path) {
        var url = this.rootUrl + path;
        var o = curl(url);
        return testlib.notEqual("404 Not Found", o.status);
    };


    this.firstLineOf = function(st) {
        var t = st.replace(/[\r\n]+/, "\n");
        var lines = t.split("\n");

        if (lines.length == 0)
            return ("");

        return (lines[0]);
    };

	if (testlib.defined.SG_NIGHTLY_BUILD || testlib.defined.SG_LONGTESTS)
    this.test404 = function() {
        var repoName = (repInfo.repoName);

        this.verifyNot404("/version.txt");
        this.verifyNot404("");
        this.verifyNot404("/");
        this.verify404("/nonexistent");
        this.verify404("/nonexistent/");

        this.verifyNot404("/repos");
        this.verifyNot404("/repos/");
        this.verify404("/repos/nonexistent.json");

        this.verifyNot404("/repos/" + repoName);
        this.verifyNot404("/repos/" + repoName + "/");
        this.verify404("/repos/" + repoName + "/nonexistent");
        this.verify404("/repos/" + repoName + "/nonexistent/");

        this.verify404("/ui/nonexistent");
        this.verifyNot404("/ui/sg.css");
        this.verify404("/ui/sg.css/nonexistent");

        if(this.serverHasWorkingCopy){
            this.verify404("/local/nonexistent");
            this.verify404("/local/nonexistent/");

            this.verifyNot404("/local/fs.json");
            this.verifyNot404("/local/fs.json");
            

            this.verifyNot404("/local/status.json");
            this.verifyNot404("/local/status.json/");
            this.verify404("/local/status.json/nonexistent");
            this.verify404("/local/status.json/nonexistent/");
        }
        else {
            this.verify404("/local");
            this.verify404("/local/");
            this.verify404("/local/nonexistent");
            this.verify404("/local/nonexistent/");
        }
    };

	this.test301 = function() {
        this.verify301("/repos/nonexistent");
        this.verify301("/repos/nonexistent/");
		
	};

    this.test405 = function() {
        var repoName = (repInfo.repoName);

       this.verify405_postdata("/repos/" + repoName + "/changesets/" + this.changesetId + ".json", true, "baddata");

    };

    this.checkHeader = function(headerName, expected, headerBlock, desc) {
        var s = headerBlock.replace(/[\r\n]+/g, "\n");
        var lines = s.split("\n");
        var headers = {};

        for (var i in lines) {
            var l = lines[i];
            var parts = l.split(": ");

            if (parts.length == 2) {
                headers[parts[0]] = parts[1];
            }
        }

        var got = headers[headerName];
        var msg = "header " + headerName + ": expected '" + expected + "', " +
	    "got '" + got + "'";

	if (!! desc)
	    msg = "(" + desc + ") " + msg;

        return (testlib.testResult(expected == got, msg));
    };

    this.testContentType = function() {
        var o1 = curl(this.rootUrl + "/repos");


        this.checkStatus("200 OK", o1.status, "error status on call 1");

        this.checkHeader("Content-Type", "text/html;charset=UTF-8", o1.headers);

        var o2 = curl(this.rootUrl + "/repos.json");
        this.checkStatus("200 OK", o2.status, "error status on call 2");
        this.checkHeader("Content-Type", "application/json;charset=UTF-8", o2.headers);
    };

    this.testListrepos = function() {
        var o = curl(this.rootUrl + "/repos.json");
        this.checkStatus("200 OK", o.status);

		/*

		Because the test suite adds garbage descriptors which the web API
		ignores but sg.list_descriptors() doesn't,  these don't work anymore.

        // Make sure all the existing repos are in the fetched list...
        var fetched_list = JSON.parse(o.body);

        var numRepos = 0;
        for (var x in sg.list_descriptors()) {
            testlib.isNotNull(fetched_list[x], "repository " + x + " should not be null");
            numRepos++;
        }

        var numFetched = 0;

        for (i in fetched_list) {
          numFetched++;
        }

        //... and that no nonexsting repos or dupes are in it.
        testlib.testResult(numFetched == numRepos);
        */
    };

    this.testAddComment = function() {
        var url = this.repoUrl + "/changesets/" + this.changesetId + "/comments.json";

        var comment = "This is a comment from the test testAddComment.";
        var o = curl(
            "-H", "From: " + this.userId,
            "-d", '{"text": "'+comment+'"}',
            (url));
        this.checkStatus("200 OK", o.status, url);

        // Test that our comment was actually added by finding it in the output of "vv log":
        o = sg.exec("vv", "history");
        testlib.equal(0, o.exit_status, "vv history exit status");
        testlib.testResult(o.stdout.search(comment) >= 0, "comment not found");
    };

    this.testAddCommentAnonymously = function() {
        var url = this.repoUrl + "/changesets/" + this.changesetId + "/comments.json";
        var comment = "This is a comment from the test testAddCommentAnonymously.";
        var o = curl("-d", '{"text": "'+comment+'"}', url);

        this.checkStatus("401 Authorization Required", o.status, url);

        // Verify that our comment was not added by searching for it in the output of "vv log":
        o = sg.exec("vv", "log");
        testlib.equal(0, o.exit_status, "vv log exit status");
        testlib.testResult(o.stdout.search(comment) < 0, "failed to find comment");
    };

    this.testFetchWorkItem = function() {
        var url = this.repoUrl + "/workitem/" + this.bugId + ".json";

		var results = this.getJsonAndHeaders(url, "work item retrieval");

		if (! results)
			return;

		var bug = results.data;

        testlib.testResult(bug.title.search("Common") >= 0);
    };


    this.testFetchSprint = function() {
        var url = this.repoUrl + "/milestone/" + this.sprintId + ".json";
        var o = curl(url);
        this.checkStatus("200 OK", o.status);
        var bug = JSON.parse(o.body);
        testlib.testResult(bug.name.search("dogfood") >= 0);
    };

    this.testFetchSprints = function() {
        var url = this.repoUrl + "/milestones.json";
        var o = curl(url);
        if (!this.checkStatus("200 OK", o.status)) return;

        var sprints = JSON.parse(o.body);

		testlib.testResult(sprints.length > 0);
    };

    this.testFetchUnreleasedSprints = function() {
		var repo = sg.open_repo(repInfo.repoName);
		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = db.begin_tx();
		var rec = ztx.new_record('sprint');
		var relsprintid = rec.recid;
		rec.description = 'to release';
		rec.name = 'to release';
		ztx.commit();

        var url = this.repoUrl + "/milestones.json";
        var o = curl(url);
        if (!this.checkStatus("200 OK", o.status)) return;;

        var sprints = JSON.parse(o.body);

		var found = {};

		for ( var i = 0; i < sprints.length; ++i )
			found[sprints[i].recid] = true;

		testlib.testResult(found[this.sprintId], 'dogfood expected');
		testlib.testResult(found[relsprintid], 'new sprint on first pass');

		ztx = db.begin_tx();
		rec = ztx.open_record('sprint', relsprintid);
		rec.releasedate = (new Date()).getTime();
		ztx.commit();

		repo.close();

        url = this.repoUrl + "/milestones.json?_unreleased=true";
        o = curl(url);

		sg.log(o.body);

        this.checkStatus("200 OK", o.status);

		sprints = JSON.parse(o.body);

		found = {};

		for ( var i = 0; i < sprints.length; ++i )
			found[sprints[i].recid] = true;

		testlib.testResult(found[this.sprintId], 'dogfood expected second pass');
		testlib.testResult(! found[relsprintid], 'new sprint not expected on second pass');
    };


    this.testAddWorkItem = function() {
        var url = this.repoUrl + "/workitems.json";
        var username = this.userId;
        var ourDesc = "This bug was added by the test testAddWorkItem.";
        var bugJson = "{"
            + "\"rectype\":\"item\","
	    + "\"id\":\"testAddWorkItem\","
            + "\"title\":\"" + ourDesc + "\"}";
        var o = curl("-H", "From: " + username, "-d", bugJson, url);

        this.checkStatus("200 OK", o.status, "posting");

        url = this.repoUrl + "/workitems.json";
        o = curl(url);
        this.checkStatus("200 OK", o.status, "retrieving records");

        var records = JSON.parse(o.body);

        if (!testlib.testResult((!!records) && (records.length > 0), "no records found"))
            return;

        var found = false;

        for (var i = 0; i < records.length; ++i) {
            if (records[i].title == ourDesc) {
                found = records[i];
                break;
            }
        }

        testlib.testResult(!!found, "no record found with matching description");
    };

    this.testAddSprint = function() {
        var url = this.repoUrl + "/milestones.json";
        var username = this.userId;
        var ourDesc = "This sprint was added by the test testAddSprint.";

		var sprintData = {
			description: ourDesc,
			name: "tas sprint"
		};
		var sprintJson = JSON.stringify(sprintData);
        var o = curl("-H", "From: " + username, "-d", sprintJson, url);

        this.checkStatus("200 OK", o.status, "posting");

        url = this.repoUrl + "/milestones.json";
        o = curl(url);
        this.checkStatus("200 OK", o.status, "retrieving records");

        var records = JSON.parse(o.body);

        if (!testlib.testResult((!!records) && (records.length > 0), "no records found"))
            return;

        var found = false;

        for (var i = 0; i < records.length; ++i) {
            if (records[i].description == ourDesc) {
                found = records[i];
                break;
            }
        }

        testlib.testResult(!!found, "no record found with matching description");
    };

    this.testAddWorkItemAnonymously = function() {
        var url = this.repoUrl + "/workitems.json";
        var bugJson = "{"
            + "\"rectype\":\"item\","
	    + "\"id\":\"testAWIA\","
            + "\"title\":\"This bug was added by the test testAddWorkItemAnnonymously.\"}";
        var o = curl("-d", bugJson, url);
        testlib.testResult(o.status.search("401 Authorization Required") >= 0);
    };

    this.testAddInvalidJsonWorkItem = function() {
        var url = this.repoUrl + "/workitems.json";
        var bugJson = "{"
            + "\"rectype\":\"item\","
	    + "\"id\":\"testAIJWI\","
            + "\"title\":\"This is not \"valid\" JSON.\"}";
        var o = curl("-H", "From: " + this.userId, "-d", bugJson, url);
        testlib.testResult(o.status=="400 Bad Request");

        // Response message's body should say something about invalid Json.
        if(sg.config.debug)
	        testlib.testResult(o.body.search("Invalid JSON.") >= 0);
    };

    this.testUpdateWorkItem = function() {
        var url = this.repoUrl + "/workitem/" + this.bugId + ".json";
		var newTitle = "Common stWeb bug updated by testUpdateWorkItem.";
		var json = sg.to_json({"title" : newTitle});
        var o = curl("-H", "From: " + this.userId, "-d", json, "-X", "PUT", url);
        this.checkStatus("200 OK", o.status);

		var r = sg.open_repo(repInfo.repoName);
		var db = new zingdb(r, sg.dagnum.WORK_ITEMS);
		var rec = db.get_record('item', this.bugId);
		testlib.equal(newTitle, rec.title, "updated title expected");
		r.close();
    };

    this.testUpdateWorkItemAnonymously = function() {
        var url = this.repoUrl + "/workitem/" + this.bugId + ".json";
		var newTitle = "Common stWeb bug updated by testUpdateWorkItemAnonymously!!";
		var json = sg.to_json({"title":newTitle});
        var o = curl("-d", json, "-X", "PUT", url);
        testlib.testResult(o.status=="401 Authorization Required");

		var r = sg.open_repo(repInfo.repoName);
		var db = new zingdb(r, sg.dagnum.WORK_ITEMS);
		var rec = db.get_record('item', this.bugId);
		testlib.notEqual(newTitle, rec.title, "title should not have been updated");
		r.close();
    };

    this.testUpdateWorkItemInvalidJson = function() {
        var url = this.repoUrl + "/workitem/" + this.bugId + ".json";
        var json = "{\"title\":\"Common stWeb bug updated by \"testUpdateWorkItemInvalidJson\"!!\"}";
        var o = curl("-H", "From: " + this.userId, "-d", json, "-X", "PUT", url);
        testlib.testResult(o.status=="400 Bad Request");

        // Response message's body should say something about invalid Json.
        if(sg.config.debug)
	        testlib.testResult(o.body.search("Invalid JSON.") >= 0);
    };

    this.testDeleteWorkItem = function() {
        var url = this.repoUrl + "/workitems.json";
        var username = this.userId;
        var bugJson = sg.to_json({rectype:"item", id:"12345678", title:"This bug was added by the test testDeleteWorkItem."});
        var o = curl("-H", "From: " + username, "-d", bugJson, url);
        this.checkStatus("200 OK", o.status);

        var bugId = o.body;

        url = this.repoUrl + "/workitem/" + bugId + ".json";

        // Try to delete the work item anonymously and verify that it fails with a 401 error:
        o = curl("-X", "DELETE", url);
        this.checkStatus("401 Authorization Required", o.status, 'anon delete attempt', o.body);


        // Now delete it for real and verify 200 OK response:
        o = curl("-X", "DELETE", "-H", "From: " + username, url);
        this.checkStatus("200 OK", o.status, 'auth post attemp', o.body);

		var repo = sg.open_repo(repInfo.repoName);
		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var rec = db.get_record('item', bugId);
		testlib.testResult(! rec, "no record expected after delete");
		repo.close();
    };

    this.assocWIwithCS = function(bugId, csId, testName) {
        var url = this.repoUrl + "/workitem/" + bugId + "/changesets.json";
        var username = this.userId;
        var json = sg.to_json({commit:csId});

        var o = curl("-H", "From: " + username, "-d", json, "-X", "PUT", url);
        return this.checkStatus("200 OK", o.status, "linking " + bugId + " to " + csId, o.body);
    };

    this.testWorkItemChangeSetAssoc = function() {
        if (!this.assocWIwithCS(this.bugId, this.changesetId, "testWorkItemChangeSetAssoc"))
            return;

		var repo = sg.open_repo(repInfo.repoName);

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			var recs = db.query('vc_changeset', ['*'], "(commit == '" + this.changesetId + "') && (item == '" + this.bugId + "')");

            testlib.equal(1, recs.length, "changeset link count");
        }
		finally
		{
			repo.close();
		}
    };

    this.testWorkItemChangeSetsReplace = function() {
        if (!this.assocWIwithCS(this.bugId, this.changesetId, "testWorkItemChangeSetAssoc"))
            return;
        if (!this.assocWIwithCS(this.bugId, this.changeset2Id, "testWorkItemChangeSetAssoc"))
            return;

		// replace with 1 & 3
        var url = this.repoUrl + "/workitem/" + this.bugId + "/changesets.json";

		var assocData = {
			"linklist" : [
				{ "commit": this.changesetId },
				{ "commit": this.changeset3Id }
			]
		};

		var jData = JSON.stringify(assocData);

        var o = curl("-H", "From: " + this.userId, "-d", jData, "-X", "PUT", url);
        this.checkStatus("200 OK", o.status, "linking");

		var found = {};

		// test 1 set, 3 set
		// test 2 not set

		var repo = sg.open_repo(repInfo.repoName);

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			var recs = db.query('vc_changeset', ['*'], "item == '" + this.bugId + "'");

			var found = {};

			for ( var i = 0; i < recs.length; ++i )
			{
				var ln = recs[i];

				found[ln.commit] = true;
            }

			testlib.ok(found[this.changesetId], "changeset 1 not linked");
			testlib.ok(found[this.changeset3Id], "changeset 3 not linked");
			testlib.ok(! found[this.changeset2Id], "changeset 2 should not be linked");
        }
		finally
		{
			repo.close();
		}
    };

    this.testRetrieveAssociations = function() {
        var url = this.repoUrl + "/workitem/" + this.bugId + "/changesets.json";

		var assocData = {
			"linklist" : [
				{ "commit": this.changesetId },
				{ "commit": this.changeset2Id }
			]
		};

		var jData = JSON.stringify(assocData);

        var o = curl("-H", "From: " + this.userId, "-d", jData, "-X", "PUT", url);
        this.checkStatus("200 OK", o.status, "linking", o.body);

        url = this.repoUrl + "/workitem/" + this.bugId + "/changesets.json";

		var results = this.getJsonAndHeaders(url, "calling for links");

        var list = results.data;

        var found1 = false;
        var found2 = false;

        for (var i in list) {
            var ln = list[i];

            if (ln.commit == this.changesetId) {
                found1 = true;
            }
            else if (ln.commit == this.changeset2Id) {
                found2 = true;
            }
        }

        testlib.ok(found1, "no changeset matched 1st " + this.changesetId);
        testlib.ok(found2, "no changeset matched 2nd " + this.changeset2Id);
    };


	this.testRetrieveAssociationsFromChangeset = function()
	{
	    var url = this.repoUrl + "/workitem/" + this.bugId + "/changesets.json";

		var assocData = {
			"linklist" : [
				{ "commit": this.changesetId }
			]
		};

		var jData = JSON.stringify(assocData);

        var o = curl("-H", "From: " + this.userId, "-d", jData, "-X", "PUT", url);
        this.checkStatus("200 OK", o.status, "linking", o.body);

        url = this.repoUrl + "/changeset/" + this.changesetId + "/workitems.json";

		var results = this.getJsonAndHeaders(url, "calling for links");

        var list = results.data;

        var found = false;

		for ( var i = 0; i < list.length; ++i )
		{
            var ln = list[i];

            if (ln.item == this.itemId) {
                found = true;
            }
        }

        testlib.ok(found, "no work item matched " + this.changesetId);

	};


    this._assertChangeSetAssociated = function(csid, bugid, bugdisplayid, expected, desc)
    {
        var repo = sg.open_repo(repInfo.repoName);
	var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
	var ztx = zs.begin_tx();

	try
	{
            var item = ztx.open_record(bugid);

	    if (testlib.testResult(!!item, "bug " + bugdisplayid + " not found")) {

		var a = item.changesets.to_array();

		var found = false;

		for (var i in a) {
		    var lid = a[i];

		    		    var ln = ztx.open_record(lid);

		    if (ln.commit == csid) {
                        found = true;
			break;
		    }
                }

		if (expected)
		    testlib.ok(found, "expected changeset match in " + bugdisplayid + " " + desc);
		else
		    testlib.ok(! found, "expected no changeset match in " + bugdisplayid + " " + desc);
            }
	}
	catch(e)
	{
	    print(e.toString());
	}

	ztx.abort();
        repo.close();
    };


    this.testChangeSetWorkItemAssoc = function()
    {
	var repo = sg.open_repo(repInfo.repoName);

        var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
        var ztx = zs.begin_tx();
        var zrec = ztx.new_record("item");
        var extrabugId = zrec.recid;
        var bugSimpleId = "123456999";
        zrec.title = "testChangeSetWorkItemAssoc extra bug.";
        zrec.id = bugSimpleId;
        ztx.commit();

	repo.close();

	var url = this.repoUrl + "/changeset/" + this.changesetId + "/workitems.json";

	var data = {
	  "locallinkname" : "items",
	  "linkname" : "vc_changeset_to_item",
	  "rectype" : "vc_changeset",
	  "linkhidfield" : "commit",

	  "linklist"   :
	    [
		{
		    "recid": this.bugId
		},
		{
		    "recid" : extrabugId
		}
	    ]
	};
	var j = JSON.stringify(data);

        var o = curl("-H", "From: " + this.userId, "-d", j, "-X", "PUT", url);
        this.checkStatus("200 OK", o.status, "linking", o.body);

	this._assertChangeSetAssociated(this.changesetId, this.bugId, this.bugSimpleId, true, "after setting both");
	this._assertChangeSetAssociated(this.changesetId, extrabugId, bugSimpleId, true, "after setting both");

	data = {
	  "locallinkname" : "items",
	  "linkname" : "vc_changeset_to_item",
	  "rectype" : "vc_changeset",
	  "linkhidfield" : "commit",

	  "linklist"   :
	    [
		{
		    "recid" : extrabugId
		}
	    ]
	};
	j = JSON.stringify(data);

        var o = curl("-H", "From: " + this.userId, "-d", j, "-X", "PUT", url);
        this.checkStatus("200 OK", o.status, "linking");

	this._assertChangeSetAssociated(this.changesetId, this.bugId, this.bugSimpleId, false, "after setting only extra");
	this._assertChangeSetAssociated(this.changesetId, extrabugId, bugSimpleId, true, "after setting only extra");
    };

    if (this.serverHasWorkingCopy) this.testLocalStatus = function ()
    {
        var url = this.rootUrl + "/local/status.json";
        var root = "statustest";

        if (sg.fs.exists(root))
        {
            sg.fs.rmdir_recursive(root);
        }

        sg.fs.mkdir(root);
        sg.fs.mkdir(root + "/subwithfiles");
        sg.file.write(root + "/subwithfiles/t1.txt", "one");
        sg.file.write(root + "/subwithfiles/t2.txt", "two");


        var o = curl(url);
        this.checkStatus("200 OK", o.status, "fail get status");

        var res = JSON.parse(o.body);
        print(o.body);
        testlib.isNotNull(res.cwd, "cwd should not be null");
        testlib.isNotNull(res.status, "status should not be null");
        // testlib.testResult(res.cwd);
    };

    if(this.serverHasWorkingCopy) this.testBrowsefsPassFail = function() {

        var root = "bfstest";

        if (sg.fs.exists(root)) {
            sg.fs.rmdir_recursive(root);
        }

        sg.fs.mkdir(root);
        sg.fs.mkdir(root + "/subexists");

        var existUrl = this.rootUrl + "/local/fs.json?path=" + root + "/subexists";
        var nonExistBase = "/local/fs.json?path=" + root + "/dne";
        var nonExistUrl = this.rootUrl + nonExistBase;

        var o = curl(existUrl);
        this.checkStatus("200 OK", o.status, "fail on existing dir: " + existUrl);

       // this.verify404(nonExistBase);

        sg.fs.rmdir_recursive(root);
    };

    if(this.serverHasWorkingCopy) this.testBrowsefsListFiles = function() {

        var root = "bfstest";

        if (sg.fs.exists(root)) {
            sg.fs.rmdir_recursive(root);
        }

        sg.fs.mkdir(root);
        sg.fs.mkdir(root + "/subwithfiles");
        sg.file.write(root + "/subwithfiles/t1.txt", "one");
        sg.file.write(root + "/subwithfiles/t2.txt", "two");

        var url = this.rootUrl + "/local/fs.json?path=" + root + "/subwithfiles";

        var o = curl(url);
        this.checkStatus("200 OK", o.status, "fail on existing dir: " + url);

        var flist = JSON.parse(o.body);

        testlib.testResult(!!flist, "file list defined");

        if (!!flist) {
            testlib.equal(2, flist.files.length, "number of files returned");

            flist.files.sort(
                function(a, b) {
                    if (a.name < b.name)
                        return (-1);
                    else if (a.name > b.name)
                        return (1);
                    else
                        return (0);
                }
            );
          
            testlib.equal("t1.txt", flist.files[0].name, "first file name");
            testlib.ok(!flist.files[0].isDirectory, "first file should not be dir");
            testlib.equal("t2.txt", flist.files[1].name, "second file name");
            testlib.ok(!flist.files[1].isDirectory, "second file should not be dir");
        }

        sg.fs.rmdir_recursive(root);
    };

    this.testAddWiComment = function() {
        var username = this.userId;
        var posturl = this.repoUrl +
	    "/workitem/" + this.bugId + "/comments.json";
        var commentText = "comment from testAddWiComment";

        var commentdata = {
            "text": commentText
        };

        var o = curl("-H", "From: " + username, "-d", sg.to_json(commentdata), "-X", "PUT", posturl);
        this.checkStatus("200 OK", o.status);

        var repo = sg.open_repo(repInfo.repoName);
        var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

		var cmts = zs.query('comment', ['*'], "item == '" + this.bugId + "'");

        repo.close();

        if (!testlib.equal(1, cmts.length, "should be 1 comment"))
            return;

        o = curl(posturl);
        this.checkStatus("200 OK", o.status, "get " + posturl);

        var d = JSON.parse(o.body);

        if (!testlib.testResult(!!d, "no data returned"))
            return;

        var found = false;

        for (var i in d) {
            if (d[i].text == commentText) {
                found = true;
                break;
            }
        }

        testlib.testResult(found, "comment was not found via web service");
    };


    this.testCommentsInActivity = function() {
        var username = this.userId;
        var posturl = this.repoUrl + "/workitem/" + this.bugId + "/comments.json";
        var commentText = "testCommentsInActivity";
        var listUrl = this.repoUrl + "/activity.json";

        var commentdata = {
            "text": commentText
        };

        var o = curl("-H", "From: " + username, "-d", sg.to_json(commentdata), "-X", "PUT", posturl);
        this.checkStatus("200 OK", o.status, "posting comment", o.body);

	var results = this.getTweets(listUrl);
        var tweets = results.tweets;
        if (!testlib.testResult(!!tweets, "no tweets found"))
            return;

        var found = false;

        for (var i = 0; i < tweets.length; ++i) {
            if (tweets[i].what.search(commentText) >= 0) {
                found = true;
                break;
            }
        }

        testlib.testResult(found, "work item comment in activity stream");
    };


    this.getTweets = function(url, ims, etag) {

        var o = false;
	var results = {
	   'tweets' : []
	};

        if (ims) {
            o = curl(url, {"If-Modified-Since": ims});
        }
        else if (etag) {
            o = curl(url, {"If-None-Match": etag});
        }
        else
            o = curl(url);

        if (o.status=="304 Not Modified") {
            return (results);
        }

        this.checkStatus("200 OK", o.status, url, o.body);

        results.headers = {};

        var s = o.headers.replace(/[\r\n]+/g, "\n");
        var lines = s.split("\n");

        for ( var i in lines )
        {
	    var parts = lines[i].split(': ');

	    results.headers[parts[0]] = parts[1];
	}

        results.tweets = JSON.parse(o.body);

        return (results);
    };

    this.testWorkItemsInActivity = function() {
		var bugid = "twiia1";
		var newbug = this.addBugWithId(bugid, "bug title " + bugid);

		var listUrl = this.repoUrl + "/activity.json";

		var results = this.getTweets(listUrl);
		var tweets = results.tweets;
		if (!testlib.testResult(!!tweets, "no tweets found"))
			return;

		var found = false;

		for (var i = 0; i < tweets.length; ++i) {
			if (tweets[i].what.search("bug title " + bugid) >= 0) {
				found = true;
				break;
			}
		}

		testlib.testResult(found, "work item expected in activity stream");
	};



	this.pad2 = function(n) {
        if (n < 10)
            return ("0" + n);

        return (n);
    };


    this.rfc850 = function(dt) {
        // Wdy, DD-Mon-YY HH:MM:SS TIMEZONE

        var days = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];
        var months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
	    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

        return (days[dt.getUTCDay()] + ", " +
	       this.pad2(dt.getUTCDate()) + " " + months[dt.getUTCMonth()] + " " +
		dt.getUTCFullYear() + " " +
	       this.pad2(dt.getUTCHours()) + ":" +
	       this.pad2(dt.getUTCMinutes()) + ":" +
	       this.pad2(dt.getUTCSeconds()) + " GMT");
    };



	/**
	 * returns :
	 *  {
	 *     headers: { (header name / value hash ) },
	 *     data: (returned content, eval'ed as JSON)
	 *  }
	 *
	 * returns false if curl is non-zero or non-200 status seen (and logs test failures)
	 */
	this.getJsonAndHeaders = function(url, desc) {
        var o = curl(url);

		desc = desc || url;

        if (!this.checkStatus("200 OK", o.status, "200 expected - " + desc))
		{
			testlib.log(o.body);
			return(false);
		}

		var jdata = JSON.parse(o.body);

		var results = {
			headers: this._parseHeaders(o.headers),
			data: jdata
		};

		return( results );
	};

    this.testHistoryJSONssjs = function() {

		var tagname = "ssjsTag";
		var stampname = "ssjsStamp";

        commitModFileCLC("testHistoryJSONssjs", stampname);

		var results = this.getJsonAndHeaders( this.repoUrl + "/history.json" );

		if (! results)
			return;

        print(sg.to_json__pretty_print(results));
		var res = results.data.items;

        var cs = res[0];

        var count = res.length;

        var csid = cs.changeset_id;
        testlib.ok(csid != null, "changeset_id should not be null");

        testlib.equal(stampname, cs.stamps[0], "stamp should be " + stampname);
        testlib.notEqual(0, cs.parents.length, "parents should not be empty");
        testlib.isNotNull(cs.audits[0].userid, "audit who should not be null");
        testlib.isNotNull(cs.audits[0].timestamp, "audit when should not be null");

        var userName = cs.audits[0].username;

        var comments = cs.comments;

        testlib.isNotNull(comments[0].text, "comments text should not be null");
        var o = sg.exec(vv, "tag", "add", tagname, "--rev=" + csid);

		if (! testlib.testResult(o.exit_status == 0, "failed to set tag on " + csid + ": " + o.body))
			return;

		results = this.getJsonAndHeaders( this.repoUrl + "/history.json" );

		if (! results)
			return;

		res = results.data.items;
        cs = res[0];

        testlib.equal(tagname, cs.tags[0], "tag");

        var dateTo = new Date(2020,11,11).getTime();
        var dateFrom = new Date(2010,04,20).getTime();

        var url = this.repoUrl + "/history.json?max=2&stamp=" + stampname + "&from=" + dateFrom + "&user="+ userName + "&to=" + dateTo;

		results = this.getJsonAndHeaders(url);

		if (! results)
			return;

		res = results.data.items;

        if (testlib.equal(1,  res.length, "length should be 1"))
		{
			testlib.equal(res[0].stamps[0], stampname, "stamp should be " + stampname);
		}

        url = this.repoUrl + "/history.json?max=0";

		results = this.getJsonAndHeaders(url);
		if (! results)
			return;

		res = results.data.items;

        testlib.equal(count,  res.length, "all items should be returned");
    };

    //TODO add test for the new way we page history
   /* this.testHistoryPaging = function() {
		var tagname = "ssjsTag";
		var stampname = "ssjsStamp";

        commitModFileCLC("testHistoryPaging1", stampname + "1");

		var r = sg.open_repo(repInfo.repoName);

		var results = r.history(1);
		var firstrevno = results[0].revno;

        commitModFileCLC("testHistoryPaging2", stampname + "2");

		results = r.history(1);
		var secondrevno = results[0].revno;

		r.close();

		results = this.getJsonAndHeaders( this.repoUrl + "/history.json" );

		if (! results)
			return;

		var res = results.data.items;

		var cs = res[0];

		if (! testlib.equal(secondrevno, cs.revno, "unqualified revision number"))
			return;

		results = this.getJsonAndHeaders( this.repoUrl + "/history.json?startrev=" + firstrevno );

		if (! results)
			return;

		res = results.data.items;

		cs = res[0];

		if (! testlib.equal(firstrevno, cs.revno, "unqualified revision number"))
			return;
    };*/

    this.testSearch = function ()
    {
        var folder = createFolderOnDisk("testGetChangesetJSON");
        var folder2 = createFolderOnDisk("testGetChangesetJSON_2");
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 9);
        var file3 = createFileOnDisk(pathCombine(folder, "file3.txt"), 5);
        var file4 = createFileOnDisk(pathCombine(folder, "file4.txt"), 10);

        vscript_test_wc__addremove();
        var csid = vscript_test_wc__commit();

        var url = this.repoUrl + "/searchresults.json?text=" + csid.slice(0, 3) + "&max=50";

        o = curl(url);
	print(o);
	print(sg.to_json__pretty_print(o));	// TODO 2012/08/30 delete me

        this.checkStatus("200 OK", o.status);
        var res = JSON.parse(o.body);
        testlib.ok(res.items.length > 0, "search should have returned results");
    };

    this.testGetChangesetJSON = function() {

        var folder = createFolderOnDisk("testGetChangesetJSON");
        var folder2 = createFolderOnDisk("testGetChangesetJSON_2");
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
        var file2 = createFileOnDisk(pathCombine(folder, "file2.txt"), 9);
        var file3 = createFileOnDisk(pathCombine(folder, "file3.txt"), 5);
        var file4 = createFileOnDisk(pathCombine(folder, "file4.txt"), 10);

        addRemoveAndCommit();
        var file5 = createFileOnDisk(pathCombine(folder, "file5.txt"), 15);

        insertInFile(file, 20);
        vscript_test_wc__remove(file3);
        vscript_test_wc__move(file2, folder2);
        vscript_test_wc__rename(file4, "file4_rename");
        vscript_test_wc__add(file5);

        expect_test = new Array();
		expect_test["Modified"] = [ "@/testGetChangesetJSON/file1.txt" ];
		expect_test["Removed"]  = [ "@b/testGetChangesetJSON/file3.txt" ];
		expect_test["Moved"]    = [ "@/testGetChangesetJSON_2/file2.txt" ];
		expect_test["Renamed"]  = [ "@/testGetChangesetJSON/file4_rename" ];
		expect_test["Added"]    = [ "@/testGetChangesetJSON/file5.txt" ];
		vscript_test_wc__confirm_wii(expect_test);

        var csid = vscript_test_wc__commit_np( { "stamp" : "stamp2",
						 "message" : "blah" } );
        var o = sg.exec(vv, "tag", "add", "tag2", "--rev=" + csid);

        var url = this.repoUrl + "/changesets/" + csid + ".json?details";

        o = curl(url);
        this.checkStatus("200 OK", o.status);

        //verify JSON results
        var cs = JSON.parse(o.body);
    
		if (1)
		{
			print("");
			print("================================================================");
			print(sg.to_json__pretty_print(cs));
			print("================================================================");
			print("");
		}

        //THIS IS WHAT THE CHANGE SET WEB PAGE EXPECTS FROM THE JSON
        //IF THIS FAILS, THAT MEANS THE WEB PAGE IS BROKEN, DON'T JUST CHANGE THE TEST

        var d = cs.description;
        testlib.isNotNull(d, "description should not be null");

        testlib.isNotNull(d.changeset_id, "description changeset_id should not be null");
        //testlib.isNotNull(d.parents, "description parents should not be null");
        testlib.isNotNull(d.audits[0].userid, "description audits userid should not be null");
        testlib.isNotNull(d.audits[0].timestamp, "description audits timestamp should not be null");
        testlib.isNotNull(d.tags, "description tags should not be null");

        //todo check valid stamp equal when stamps work in jsglue
        testlib.isNotNull(d.stamps, "description stamps should not be null");
        //testlib.isNotNull(d.children, "description children should not be null");
        testlib.isNotNull(d.comments, "description comments should not be null");

        testlib.equal(d.comments[0].text, "blah");
        testlib.isNotNull(d.comments[0].history[0].audits[0].userid, "description comments userid should not be null");
        testlib.isNotNull(d.comments[0].history[0].audits[0].timestamp, "description comments timestamp should not be null");

	// TODO 2012/08/31 Is really the best way to do this?
	var parents = new Array();
	var p_k = 0;
	for (var p in d.parents)
	{
	    parents[p_k] = p;
	    p_k = p_k + 1;
	}
	// NOTE 2012/08/31 With the W8712 changes, getchangeset.js now 
	// NOTE            returns canonical STATUS info rather than MSTATUS.
	//
	// NOTE 2012/11/07 With the W3440,W9284 changes, we report historical
	// NOTE            status using @0/ and @1/ prefixes.

        expect_test = new Array();
        expect_test["Modified"] = [ "@1/testGetChangesetJSON/file1.txt" ];
        expect_test["Removed"]  = [ "@0/testGetChangesetJSON/file3.txt" ];
        expect_test["Moved"]    = [ "@1/testGetChangesetJSON_2/file2.txt" ];
        expect_test["Renamed"]  = [ "@1/testGetChangesetJSON/file4_rename" ];
        expect_test["Added"]    = [ "@1/testGetChangesetJSON/file5.txt" ];
        vscript_test_wc__confirm(expect_test, cs[parents[0]]);

        //test .html page
        o = curl(url);
        this.checkStatus("200 OK", o.status);
    };



    this.testBrowseChangeSetJSON = function testBrowseChangeSetJSON()
    {

        var folder = createFolderOnDisk("testBrowseChangeSetJSON");
        var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);

        vscript_test_wc__addremove();

        var csid = vscript_test_wc__commit();

        var csurl = this.repoUrl + "/changesets/" + csid + ".json?details";

        //THIS IS WHAT THE BROWSE CHANGE SET WEB PAGE EXPECTS FROM THE JSON
        //IF THIS FAILS, THAT MEANS THE WEB PAGE IS BROKEN, DON'T JUST CHANGE THE TEST

        //test folder GID
        var o = curl(csurl);
        this.checkStatus("200 OK", o.status, "status of csid request", o.body);

        var cs = JSON.parse(o.body);
		if (0)
		{
			print("");
			print("================================================================");
			print("Test folder GID");
			print(sg.to_json__pretty_print(cs));
			print("================================================================");
			print("");
		}

        var gid = 0;

        var pcsid;
        for (var p in cs.description.parents)
        {
            pcsid = p;

        }
        //we don't know which value is gid is returned first, we want the Directory
        for (var i = 0; i < cs[pcsid].length; i++)
        {
                if (cs[pcsid][i].status.isDirectory)
                        gid = cs[pcsid][i].gid;
        }

        var bcsurl = this.repoUrl + "/changesets/" + csid + "/browsecs.json?gid=" + gid;
        print(bcsurl);
        o = curl(bcsurl);
        this.checkStatus("200 OK", o.status);

        cs = JSON.parse(o.body);
		if (0)
		{
			print("");
			print("================================================================");
			print("Test browsecs");
			print(sg.to_json__pretty_print(cs));
			print("================================================================");
			print("");
		}

        testlib.equal(cs.name, "testBrowseChangeSetJSON", "name should be testBrowseChangeSetJSON");
        testlib.isNotNull(cs.changeset_id, "changeset_id should not be null");
        testlib.equal(cs.path, "@/testBrowseChangeSetJSON", "path should be @/testBrowseChangeSetJSON");
        testlib.equal(cs.type, "Directory", "type should be directory");
        testlib.isNotNull(cs.hid, "hid should not be null");
        testlib.isNotNull(cs.gid, "gid should not be null");

        var ct = cs.contents[0];
        testlib.isNotNull(ct, "contents should not be null");
        testlib.isNotNull(ct.gid, "contents gid should not be null");
        testlib.isNotNull(ct.hid, "contents hid should not be null");
        testlib.equal(ct.name, "file1.txt", "contents name should be file1.txt");
        testlib.equal(ct.type, "File", "type should be File");
        testlib.equal(ct.path, "@/testBrowseChangeSetJSON/file1.txt", "path should be @/testBrowseChangeSetJSON/file1.txt");


        //test folder Path
        bcsurl = this.repoUrl + "/changesets/" + csid + "/browsecs.json?path=@/testBrowseChangeSetJSON";
        o = curl(bcsurl);
        this.checkStatus("200 OK", o.status);


        var cs1 = JSON.parse(o.body);
		if (0)
		{
			print("");
			print("================================================================");
			print("Test cs1");
			print(sg.to_json__pretty_print(cs1));
			print("================================================================");
			print("");
		}

        testlib.equal(cs1.name, "testBrowseChangeSetJSON", "name should be testBrowseChangeSetJSON");
        testlib.isNotNull(cs1.changeset_id, "changeset_id should not be null");
        testlib.equal(cs1.path, "@/testBrowseChangeSetJSON", "path should be @/testBrowseChangeSetJSON");
        testlib.equal(cs1.type, "Directory", "type should be directory");
        testlib.isNotNull(cs1.hid, "hid should not be null");
        testlib.isNotNull(cs1.gid, "gid should not be null");

        var ct1 = cs1.contents[0];
        testlib.isNotNull(ct1, "contents should not be null");
        testlib.isNotNull(ct1.gid, "contents gid should not be null");
        testlib.isNotNull(ct1.hid, "contents hid should not be null");
        testlib.equal(ct1.name, "file1.txt", "contents name should be file1.txt");
        testlib.equal(ct1.type, "File", "type should be File");
        testlib.equal(ct1.path, "@/testBrowseChangeSetJSON/file1.txt", "path should be @/testBrowseChangeSetJSON/file1.txt");


        //test file
        bcsurl = this.repoUrl + "/changesets/" + csid + "/browsecs.json?path=@/testBrowseChangeSetJSON/file1.txt";
        o = curl(bcsurl);
        this.checkStatus("200 OK", o.status);

        var csFile = JSON.parse(o.body);

        testlib.equal(csFile.name, "file1.txt", "name should be file1.txt");
        testlib.isNotNull(csFile.changeset_id, "changeset_id should not be null");
        testlib.equal(csFile.path, "@/testBrowseChangeSetJSON/file1.txt", "path should be @/testBrowseChangeSetJSON/file1.txt");
        testlib.equal(csFile.type, "File", "type should be File");
        testlib.isNotNull(csFile.hid, "hid should not be null");
        testlib.isNotNull(csFile.gid, "gid should not be null");

        //test .html page path

        bcsurl = this.repoUrl + "/changesets/" + csid + "/browsecs.json?path=@/testBrowseChangeSetJSON";
        // print("testing " + bcsurl);
        o = curl(bcsurl);
        this.checkStatus("200 OK", o.status);

        //test .html page gid
        bcsurl = this.repoUrl + "/changesets/" + csid + "/browsecs.json?gid=" + (gid);
        // print("testing " + bcsurl);
        o = curl(bcsurl);
        this.checkStatus("200 OK", o.status);
    };

    this.testDiffLocal = function testDiffLocal()
    {
        if (this.serverHasWorkingCopy)
        {
            var file = createFileOnDisk("testDiffLocal.txt", 10);
            vscript_test_wc__add(file);

            var csid = vscript_test_wc__commit();
            insertInFile(file, 20);
            var csurl = this.repoUrl + "/changesets/" + csid + ".json?details";

            var o = curl(csurl);
            var cs = JSON.parse(o.body);
			if (1)
			{
				print("");
				print("================================================================");
				print("Test changeset detail");
				print(sg.to_json__pretty_print(cs));
				print("================================================================");
				print("");
			}

            var pcsid;
            for (var p in cs.description.parents)
            {
                pcsid = p;
            }

            testlib.ok( (cs[pcsid][0].status.isAdded) );
            var hid = cs[pcsid][0].B.hid;
            var localFolder = pathCombine("@", file);
            diffurl = this.repoUrl + "/diff_local.json?hid1=" + hid + "&path=" + localFolder;
            print("testing " + diffurl);

            o = curl(diffurl);
            print(o.body);
            this.checkStatus("200 OK", o.status);

        }


    };

    this.testDiff = function testDiff()
    {
        var file = createFileOnDisk("testDiff.txt", 10);
        addRemoveAndCommit();
        insertInFile(file, 20);
        var csid = vscript_test_wc__commit();

        var csurl = this.repoUrl + "/changesets/" + csid + ".json?details";

        var o = curl(csurl);
        var cs = JSON.parse(o.body);
		if (1)
		{
			print("");
			print("================================================================");
			print("Test changeset detail");
			print(sg.to_json__pretty_print(cs));
			print("================================================================");
			print("");
		}

        var pcsid;
        for (var p in cs.description.parents)
        {
            pcsid = p;
        }

        testlib.ok( (cs[pcsid][0].status.isNonDirModified) );
        var mod_old_hid = cs[pcsid][0].A.hid;
        var mod_hid     = cs[pcsid][0].B.hid;

        var diffurl = "/diff.json?hid1=" + mod_hid;
        // print ("testing " + diffurl);
        this.verify404(diffurl);

        diffurl = this.repoUrl + "/diff.json?hid1=" + mod_hid + "&hid2=" + mod_old_hid;
        // print ("testing " + diffurl);
        o = curl(diffurl);
        this.checkStatus("200 OK", o.status);

        diffurl = this.repoUrl + "/diff.json?hid1=" + mod_hid + "&hid2=" + mod_old_hid + "&file=testDiff.txt";
        // print ("testing " + diffurl);

        o = curl(diffurl);
        this.checkStatus("200 OK", o.status);


    };


    this.testGetComments = function testGetComments() {

        var file = createFileOnDisk("testGetComments", 10);
        vscript_test_wc__add(file);
        var csid = vscript_test_wc__commit("comment");

        var csurl = this.repoUrl + "/changesets/" + csid + "/comments.json";

        var o = curl("-G", csurl);
        this.checkStatus("200 OK", o.status, null, o.body);
        var cs = JSON.parse(o.body);

        testlib.equal(cs[0].text, "comment");
        testlib.isNotNull(cs[0].history[0].audits[0].userid, "userid should not be null");
        testlib.isNotNull(cs[0].history[0].audits[0].timestamp, "timestamp should not be null");
        // 'when' is an int64, NOT a formatted string
        // 'whenint64' does not exist
        //testlib.isNull(cs[0].whenint64, "whenint64 comments should not exist");

    };

  this.testGetAllStamps = function testGetAllStamps() {

        var o = curl(this.repoUrl + "/stamps.json/");
        res = JSON.parse(o.body);

        var cs = res[0];

        testlib.isNotNull(cs.stamp, "stamp should not be null");
  };

  this.testPostStamp = function testPostStamp() {

        var url = this.repoUrl + "/changesets/" + this.changesetId + "/stamps.json";

        var stamp = "testPostStamp";
        var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + stamp + '"}', url);
        this.checkStatus("200 OK", o.status, url);

        // Test that our stmap was actually added
        o = sg.exec("vv", "stamp", "list");
        if (!testlib.testResult(o.exit_status == 0, "failed vv")) return;
        testlib.testResult(o.stdout.search(stamp) >= 0, "stamp not found");


  };


  this.testDeleteStamp = function testDeleteStamp() {

        var stamp = "testDeleteStamp";
        var url = this.repoUrl + "/changesets/" + this.changesetId + "/stamps.json";


        var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + stamp + '"}', url);
        this.checkStatus("200 OK", o.status, url);

        // Test that our stmap was actually added
        o = sg.exec("vv", "stamp", "list");
        if (!testlib.testResult(o.exit_status == 0, "failed vv")) return;
        testlib.testResult(o.stdout.search(stamp) >= 0, "stamp should be in list");

        url = this.repoUrl + "/changesets/" + this.changesetId + "/stamps/" + stamp;

        o = curl("-H", "From: " + this.userId, "-X", "DELETE", url);
        this.checkStatus("200 OK", o.status, url);

        // Test that our comment was actually deleted
        o = sg.exec("vv", "stamp", "list");
        if (!testlib.testResult(o.exit_status == 0, "failed vv")) return;
        // print (o.body);
        testlib.testResult(o.stdout.search(stamp) < 0, "stamp should be gone");


  };

  this.testPostTag = function testPostTag() {

        var url = this.repoUrl + "/changesets/" + this.changesetId + "/tags.json";

        var tag = "testPostTag";
        var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + tag + '"}', url);
        this.checkStatus("200 OK", o.status, url);

        // Test that our stmap was actually added
        o = sg.exec("vv", "tag", "list");
        if (!testlib.testResult(o.exit_status == 0, "failed vv")) return;
        testlib.testResult(o.stdout.search(tag) >= 0, "tag should be in list");


  };

  this.testDeleteTag = function testDeleteTag() {

        var url = this.repoUrl + "/changesets/" + this.changesetId + "/tags.json";

        var tag = "testDeleteTag";
        var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + tag + '"}', url);
        this.checkStatus("200 OK", o.status, url);

        // Test that our stmap was actually added
        o = sg.exec("vv", "tag", "list");
        if (!testlib.testResult(o.exit_status == 0, "failed vv")) return;
        testlib.testResult(o.stdout.search(tag) >= 0, "tag not found");

        url = this.repoUrl + "/changesets/" + this.changesetId + "/tags/" + tag;

        o = curl("-H", "From: " + this.userId, "-X", "DELETE", url);
        this.checkStatus("200 OK", o.status, url);

        // Test that our tag was actually added
        o = sg.exec("vv", "tag", "list");
        if (!testlib.testResult(o.exit_status == 0, "failed vv")) return;
        testlib.testResult(o.stdout.search(tag) <  0, "tag should be gone");

  };


  this.testHistoryQuery = function testHistoryQuery() {

    //todo

  };

  this.testGetCSTags = function testGetCSTags() {

       var folder = createFolderOnDisk("testGetCSTags");
       var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);

       vscript_test_wc__addremove();

       var csid = vscript_test_wc__commit();

       var url = this.repoUrl + "/changesets/" + csid + "/tags.json";

       var tag = "testGetCSTags";
       var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + tag + '"}', url);
       this.checkStatus("200 OK", o.status, url);


       o = curl(url);
       this.checkStatus("200 OK", o.status, url);

       var res = JSON.parse(o.body);

       testlib.equal(1, res.length, "should be one tag");
       var cs = res[0];
       testlib.equal(tag, cs.tag, "tag should be testGetCSTags");

  };

  //todo make this work again
  this.testTagsSpaces = function testTagsSpaces()
  {

      var folder = createFolderOnDisk("testTagsSpaces");
      var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);

      vscript_test_wc__addremove();

      var csid = vscript_test_wc__commit();

      var url = this.repoUrl + "/changesets/" + csid + "/tags.json";

      var tag = "   testTagSpaces  ";
      var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + tag + '"}', url);
      this.checkStatus("200 OK", o.status, url);


      o = curl(url);
      this.checkStatus("200 OK", o.status, url);

      var res = JSON.parse(o.body);
       print (o.body);

      testlib.equal(1, res.length, "should be one tag");
      var cs = res[0];
      testlib.equal("testTagSpaces", cs.tag, "tag should be testGetCSTags");

      tag = "testTagSpaces";
      //this should pass because its a duplicate of the same tag
      o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + tag + '"}', url);
      this.checkStatus("200 OK", o.status, url);

      res = JSON.parse(o.body);

      testlib.equal("testTagSpaces", cs.tag, "no duplicate - tag should be testTagSpaces");

      //this should return a duplicate error
      folder = createFolderOnDisk("testTagsSpaces2");
      file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);

      vscript_test_wc__addremove();

      csid = vscript_test_wc__commit();

      url = this.repoUrl + "/changesets/" + csid + "/tags.json";
     
      o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + tag + '"}', url);
      this.checkStatus("200 OK", o.status, url);
      var p = o.body;
      testlib.testResult(p.indexOf("description") >= 0, "should be duplicate");
      //testlib.equal(tag, res2.duplicate, "duplicate should be testTagSpaces");
  };

  this.testGetCSStamps = function testGetCSStamps() {
       var folder = createFolderOnDisk("testGetCSStamps");
       var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);

       vscript_test_wc__addremove();
       var csid = vscript_test_wc__commit();

       var url = this.repoUrl + "/changesets/" + csid + "/stamps.json";

       var stamp = "testGetCSStamps";
       var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + stamp + '"}', url);
       this.checkStatus("200 OK", o.status, url);


       o = curl(url);
       this.checkStatus("200 OK", o.status, url);

       var res = JSON.parse(o.body);

       testlib.equal(1, res.length, "should be one stamp");
       var cs = res[0];
       testlib.equal( stamp, cs.stamp, "stamp should be testGetCSStamps");

  };

    this.testStampSpaces = function testStampSpaces() {
       var folder = createFolderOnDisk("testStampSpaces");
       var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);

       vscript_test_wc__addremove();
       var csid = vscript_test_wc__commit();

       var url = this.repoUrl + "/changesets/" + csid + "/stamps.json";

       var stamp = "  testStampSpaces  ";
       var o = curl("-H", "From: " + this.userId, "-d", '{"text": "' + stamp + '"}', url);
       this.checkStatus("200 OK", o.status, url);


       o = curl(url);
       this.checkStatus("200 OK", o.status, url);

       var res = JSON.parse(o.body);

       testlib.equal(1, res.length, "should be one stamp");
       var cs = res[0];
       testlib.equal( "testStampSpaces", cs.stamp, "stamp should be testGetCSStamps");

  };


	this.testGetBlob = function testGetBlob() {
		var repo = sg.open_repo(repInfo.repoName);
		var blobfile = repo.fetch_blob_into_tempfile(this.changesetId, true);
		repo.close();

        var bloburl;
		var o;

		bloburl =  this.repoUrl + "/blobs/" + this.changesetId;
		// print(bloburl);
        o = curl(bloburl);
        if(this.checkStatus("200 OK", o.status)) {
			testlib.testResult(o.headers.search("Content-Disposition: attachment")<0);
			sg.file.write("temp.txt", o.body);
			testlib.testResult(compareFiles(blobfile.path, "temp.txt"));
			sg.fs.remove("temp.txt");
		}

        bloburl =  this.repoUrl + "/blobs/" + this.changesetId + "/cs.txt";
		// print(bloburl);
        o = curl(bloburl);
        if(this.checkStatus("200 OK", o.status)) {
			testlib.testResult(o.headers.search("Content-Disposition: attachment")<0);
			sg.file.write("temp.txt", o.body);
			testlib.testResult(compareFiles(blobfile.path, "temp.txt"));
			sg.fs.remove("temp.txt");
		}

        bloburl =  this.repoUrl + "/blobs-download/" + this.changesetId;
		// print(bloburl);
        o = curl(bloburl);
        if(this.checkStatus("200 OK", o.status)) {
			testlib.testResult(o.headers.search("Content-Disposition: attachment")>=0);
			testlib.testResult(o.headers.search("Content-Disposition: attachment; filename=")<0);
			sg.file.write("temp.txt", o.body);
			testlib.testResult(compareFiles(blobfile.path, "temp.txt"));
			sg.fs.remove("temp.txt");
		}

        bloburl =  this.repoUrl + "/blobs-download/" + this.changesetId + "/cs.txt";
		// print(bloburl);
        o = curl(bloburl);
        if(this.checkStatus("200 OK", o.status)) {
			testlib.testResult(o.headers.search("Content-Disposition: attachment; filename=cs.txt")>=0);
			sg.file.write("temp.txt", o.body);
			testlib.testResult(compareFiles(blobfile.path, "temp.txt"));
			sg.fs.remove("temp.txt");
		}

		sg.fs.remove(blobfile.path);
    };

    this.testWorkItemRoundTrip = function() {
		var url = this.repoUrl + "/workitems.json";
        var username = this.userId;
		var ourDesc = "This bug was added by the test testWorkItemRoundTrip.";
        var bugJson = "{"
            + "\"rectype\":\"item\","
			+ "\"id\":\"testAddWIRT\","
			+ "\"listorder\":1,"
            + "\"title\":\"" + ourDesc + "\"}";
        var o = curl("-H", "From: " + username, "-d", bugJson, url);

        this.checkStatus("200 OK", o.status, "initial post");

		var wid = o.body;

        o = curl(url);
        this.checkStatus("200 OK", o.status, "retrieval");

        var records = JSON.parse(o.body);

        if (!testlib.testResult((!!records) && (records.length > 0), "no records found"))
            return;

		var rec = false;

		for ( var i = 0; i < records.length; ++i )
		{
			if (records[i].recid == wid)
			{
				rec = records[i];
				break;
			}
		}

		if (! testlib.testResult(!! rec, "no matching record"))
			return;

		rec.listorder = parseInt(rec.listorder);

		delete rec.rectype;
		var jUpdate = JSON.stringify(rec);

        o = curl(
						"-H", "From: " + username,
						"-X", "PUT",
						"-d", jUpdate,
						this.repoUrl + "/workitem/" + wid + ".json");

        this.checkStatus("200 OK", o.status, "update", o.body);

	// todo: once working at all, modify and test an integer value
    };

    this.testGetTags = function testGetTags() {

        var file = createFileOnDisk("testGetTags.txt", 20);

        vscript_test_wc__addremove();

        var csid = vscript_test_wc__commit();

        var o = sg.exec(vv, "tag", "add", "testGetTags", "--rev=" + csid);
        o = curl(this.repoUrl + "/tags.json");
        var res = JSON.parse(o.body);

        var cs = res[0];
        testlib.isNotNull(cs.tag, "tag should not be null");
        testlib.isNotNull(cs.csid, "csid should not be null");

   };

	this._getBug = function(recid) {
		var repo = false;
		var rec = false;

		try
		{
			repo = sg.open_repo(repInfo.repoName);
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			rec = db.get_record('item', recid, ["*"]);
		}
		finally
		{
			if (repo)
				repo.close();
		}

		return(rec);
	};

    this.testAssignDuplicateTwice = function() {
		var dupid = this.addBugWithId("tadt1");

        var url = this.repoUrl + "/workitem/" + this.bugId + "/duplicateof.json";

        var username = this.userId;
		var testname = "testAssignDuplicateTwice";

        var linkdata = {
            "recid": dupid
        };

        var o = curl("-H", "From: " + username, "-X", "PUT", "-d", JSON.stringify(linkdata), url);

        this.checkStatus("200 OK", o.status, "linking " + this.bugId + " to " + dupid);

		var q = "(target == '" + dupid + "') && (source == '" + this.bugId + "')";
		var dups = this.query('relation', q);

		if (! testlib.equal(1, dups.length, "duplicates after first setting"))
			return(false);

        o = curl("-H", "From: " + username, "-X", "PUT", "-d", JSON.stringify(linkdata), url);
        this.checkStatus("200 OK", o.status, "relinking " + this.bugId + " to " + this.sprintId);

		dups = this.query('relation', q);
		testlib.equal(1, dups.length, "duplicates after second setting");
	};

	this.query = function(rectype, q)
	{
		q = q || null;

		var repo = sg.open_repo(repInfo.repoName);

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			var results = db.query(rectype, ['*'], q);
			return(results);
		}
		finally
		{
			if (repo)
				repo.close();
		}
	}

    this.testAssignToSprintImmediate = function() {

        var url = this.repoUrl + "/workitems.json";
        var username = this.userId;
        var testname = "testAssignToSprintImmediate";

	var rec = {
	    "rectype": "item",
	    "id": "tatsi",
	    "title": testname,
	    "milestone": this.sprintId
	};

	var bugJson = JSON.stringify(rec);

        var o = curl(
            "-H", "From: " + username,
            "-d", bugJson,
            url);

        if (!this.checkStatus("200 OK", o.status, "post data", o.body))
            return (false);

		var recid = o.body;

        url = this.repoUrl + "/workitem/" + recid + ".json";

        o = curl(url);
        if (!this.checkStatus("200 OK", o.status, "retrieval")) return;

        rec = JSON.parse(o.body);

		testlib.equal(this.sprintId, rec.milestone, "sprint in returned record");
    };



    this.testListUsers = function() {
        var url = this.repoUrl + "/users.json";

        var testname = "testListUsers";
        var username = this.userId;

        var o = curl("-H", "From: " + username, url);
        !this.checkStatus("200 OK", o.status, "listing users");

        var reclist = JSON.parse(o.body);

        if (testlib.testResult(!! reclist, "user list defined"))
            if (testlib.testResult(reclist.length > 0, "no users seen"))
            {
                for ( var i = 0; i < reclist.length; ++i )
                {
                    var email = reclist[0].name;
                    // print("email: " + email + "\n");
                }
            }
    };

    this.testUpdateToDeath = function() {
        var uData = {
            "id": this.bugSimpleId,
            "title": "second work item",
            "status" : "open",
            "priority" : "High"
        };
        var updateUrl = this.repoUrl + "/workitem/" + this.bugId + ".json";
	    var uStr = JSON.stringify(uData);
        var username = this.userId;

        for ( var i = 0; i < 5; ++i )
        {
            var o = curl(
                            "-H", "From: " + username,
	                    "-X", "PUT",
                            "-d", uStr,
                            updateUrl);

            if (!this.checkStatus("200 OK", o.status, "update " + i, o.body)) return;
        }
    };

    this.testSearchWorkItems = function() {
	this.searchables = {};
	this._addSearchable("this is here", true);
	this._addSearchable("and this", true);
	this._addSearchable("and this, too", true);
	this._addSearchable( "but not that", false);

        var url = this.repoUrl + "/workitems.json?q=this";
        var o = curl(url);
        if (!this.checkStatus("200 OK", o.status, "retrieving records")) return;

        var records = JSON.parse(o.body);

	var found = {};

	for ( var i = 0; i < records.length; ++i )
	{
	    var rec = records[i];

	    found[rec.recid] = true;
	}

	for ( var recid in this.searchables )
	{
	    var shouldFind = this.searchables[recid];

	    if (shouldFind)
		testlib.testResult(!! found[recid], "expected to find rec " + recid);
	    else
		testlib.testResult(! found[recid], "expected not to find rec " + recid);
	}
    };


    this.testAttachFileToWorkItem = function() {
        var updateUrl = this.repoUrl + "/workitem/" + this.bugId + "/attachments";
        var username = this.userId;

		var fileData = "\r\ntesting\r\n\r\n";
		var fn = "testAttachFileToWorkitem.txt";
		sg.file.write(fn, fileData);

        var o = curl(
                        "-H", "From: " + username,
						"-X", "POST",
                        "-F", "file=@" + fn,
                        updateUrl);

        if (!this.checkStatus("200 OK", o.status, "posting attachment", o.body))
			return;

		var recid = o.body;

		var url = this.repoUrl + "/workitem/" + this.bugId + ".json?_fields=*,!";

		var results = this.getJsonAndHeaders(url, "retrieving work item");

		if (! results)
			return;

		var bug = results.data;

		if (! testlib.testResult(bug.attachments && (bug.attachments.length > 0), "attachments expected"))
			return;

		var atts = bug.attachments;

		var found = false;
		var att = null;
		var blobdata = null;

		for ( var i = 0; i < atts.length; ++i )
			if (recid == atts[i])
			{
				found = true;
				var repo = sg.open_repo(repInfo.repoName);
				var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
				att = db.get_record('attachment', atts[i]);

				var blobfile = repo.fetch_blob_into_tempfile(att.data, true);
				repo.close();

				blobdata = sg.file.read(blobfile.path);
				
				break;
			}

		if (testlib.testResult(found, "recid not found in attachment list"))
		{
			testlib.equal('text/plain', att.contenttype, "content type");
			testlib.equal(fn, att.filename, 'file name');
			testlib.equal(fileData, blobdata, 'contents');
		}
    };

    this.testRetrieveAttachment = function() {
        var updateUrl = this.repoUrl + "/workitem/" + this.bugId + "/attachments";
        var username = this.userId;

	var fileData = "testing";
	var fn = "testRetrieveAttachment.txt";
	sg.file.write(fn, fileData);

        var o = curl(
                        "-H", "From: " + username,
	                "-X", "POST",
                        "-F", "file=@" + fn,
                        updateUrl);

        if (!this.checkStatus("200 OK", o.status, "posting attachment"))
	    return;

	var recid = o.body;

	var retrieveUrl = this.repoUrl + "/workitem/" + this.bugId + "/attachment/" + recid + ".json";

        o = curl("-H", "From: " + username, retrieveUrl);

        this.checkStatus("200 OK", o.status, "retrieving attachment");

		var data = o.body;

		testlib.testResult(data.match(fileData), "file data");
    };

    this.addBugWithId = function(id, title) {

	var result = false;

        var repo = sg.open_repo(repInfo.repoName);
	var ztx = false;

	try
        {
            var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
            ztx = zs.begin_tx();

            var zrec = ztx.new_record("item");
            zrec.title = title || ("bug " + id);
	    zrec.id = id;

	    result = zrec.recid;

            ztx.commit();
	}
	catch (e)
	{
	    testlib.fail(e.toString());

	    if (ztx)
		ztx.abort();

	    repo.close();

	    return(false);
	}

	repo.close();

	return(result);
    };

    this.testSortRecords = function() {
       	var qUrl = this.repoUrl + "/workitems.json";

	var recOneId = "A0001";
	var recTwoId = "A0002";
	var recThreeId = "B0001";

	if (! (this.addBugWithId(recOneId) && this.addBugWithId(recTwoId) && this.addBugWithId(recThreeId)))
	    return;

	var forwardUrl = qUrl + "?_sort=id";
	var revUrl = qUrl + "?_sort=id&_desc=1";

	var foundOrder = {};

        var o = curl(forwardUrl);
        if (!this.checkStatus("200 OK", o.status, "retrieving forward")) return;

        var records = JSON.parse(o.body);

	for ( var i = 0; i < records.length; ++i )
	{
	    var rec = records[i];

	    foundOrder[rec.id] = i;
	}

        testlib.testResult(foundOrder[recOneId] < foundOrder[recTwoId], recOneId + " before " + recTwoId);
        testlib.testResult(foundOrder[recOneId] < foundOrder[recThreeId], recOneId + " before " + recThreeId);
        testlib.testResult(foundOrder[recTwoId] < foundOrder[recThreeId], recTwoId + " before " + recThreeId);

	foundOrder = {};

        o = curl(revUrl);
        this.checkStatus("200 OK", o.status, "retrieving backward");

        var records = JSON.parse(o.body);

	for ( var i = 0; i < records.length; ++i )
	{
	    var rec = records[i];

	    foundOrder[rec.id] = i;
	}

        testlib.testResult(foundOrder[recOneId] > foundOrder[recTwoId], recOneId + " after " + recTwoId);
        testlib.testResult(foundOrder[recOneId] > foundOrder[recThreeId], recOneId + " after " + recThreeId);
        testlib.testResult(foundOrder[recTwoId] > foundOrder[recThreeId], recTwoId + " after " + recThreeId);
    };

    this.testPullLinkedComments = function()
	{
        var repo = sg.open_repo(repInfo.repoName);
		var cmtRecId = false;
		var ztx = false;

		try
		{
			var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			ztx = zs.begin_tx();

			var zrec = ztx.new_record("comment");
			cmtRecId = zrec.recid;
			zrec.text = "comment on " + this.bugSimpleId;
			zrec.item = this.bugId;
			ztx.commit();
		}
		catch (e)
		{
			testlib.fail(e.toString());

			if (ztx)
				ztx.abort();

			repo.close();

			return;
		}

		repo.close();

        var url = this.repoUrl + "/workitems.json?_includelinked=1&_fields=*";

        var o = curl("-H", "From: " + this.userId, url);

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status, url)))
            return;

		var found = false;

		var recs = JSON.parse(o.body);

        for ( var i = 0; i < recs.length; ++i )
		{
			var rec = recs[i];

			if (rec.recid == cmtRecId)
			{
				found = true;
				break;
			}
		}

		testlib.testResult(found, "record " + cmtRecId + " not found");

        url = this.repoUrl + "/workitems.json/?rectype=item";

        o = curl("-H", "From: " + this.userId, url);

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

		found = false;

		recs = JSON.parse(o.body);

        for ( var i = 0; i < recs.length; ++i )
		{
			var rec = recs[i];

			if (rec.recid == cmtRecId)
			{
				found = true;
				break;
			}
		}

		testlib.testResult(! found, "record " + cmtRecId + " should not be found");
    };

    this.testPullLinkedCommentsSingle = function() {
        var repo = sg.open_repo(repInfo.repoName);
		var cmtRecId = false;
		var ztx = false;
		var recid = this.bugId;

		try
        {
            var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
            ztx = zs.begin_tx();

            var zrec = ztx.new_record("comment");
			cmtRecId = zrec.recid;
			zrec.text = "comment on " + this.bugSimpleId;
			zrec.item = this.bugId;
            ztx.commit();
		}
		catch (e)
		{
			testlib.fail(e.toString());

			if (ztx)
				ztx.abort();

			repo.close();

			return;
		}

		repo.close();

        var url = this.repoUrl + "/workitem/" + recid + ".json?_includelinked=1";

        var o = curl("-H", "From: " + this.userId, url);

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status, "retrieving after link", o.body)))
            return;

		var found = false;

		var recs = JSON.parse(o.body);

        for ( var i = 0; i < recs.length; ++i )
		{
			var rec = recs[i];

			if (rec.recid == cmtRecId)
			{
				found = true;
				break;
			}
		}

		testlib.testResult(found, "record " + cmtRecId + " not found");

    };

    this.testPagedRecords = function() {
	var recIds = [ "TPR0001", "TPR0002", "TPR0003", "TPR0004", "TPR0005" ];

	for ( var i in recIds )
	{
	    var rid = recIds[i];

	    if (! this.addBugWithId(rid))
		return;
	}

	var recidsSeen = {};
	var _size = function(obj) {
	    var size = 0, key;

	    for (key in obj) {
		if (obj.hasOwnProperty(key))
		    size++;
	    }
	    return size;
	};

        var url = this.repoUrl + "/workitems.json?_sort=id&_returnmax=2&_skip=0";

        var o = curl("-H", "From: " + this.userId, url);

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

	var recs = JSON.parse(o.body);

	if (! testlib.equal(2, recs.length, "record count on first call"))
	    return;

        for ( var i = 0; i < recs.length; ++i )
	{
	    var rec = recs[i];

	    if (! recidsSeen[rec.id])
		recidsSeen[rec.id] = 0;

	    recidsSeen[rec.id]++;
	}

	if (! testlib.equal(2, _size(recidsSeen), "recid count on first call"))
	    return;

        url = this.repoUrl + "/workitems.json?_sort=id&_returnmax=2&_skip=2";

        var o = curl("-H", "From: " + this.userId, url);

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

	recs = JSON.parse(o.body);

	if (! testlib.equal(2, recs.length, "record count on second call"))
	    return;

        for ( i = 0; i < recs.length; ++i )
	{
	    var rec = recs[i];

	    if (! recidsSeen[rec.id])
		recidsSeen[rec.id] = 0;

	    recidsSeen[rec.id]++;
	}

	if (! testlib.equal(4, _size(recidsSeen), "recid count on second call"))
	    return;

	// get the rest
        url = this.repoUrl + "/workitems.json?_sort=id&_skip=4&_returnmax=500000";

        var o = curl(
			"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
			(url)
		       );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

	recs = JSON.parse(o.body);

        for ( i = 0; i < recs.length; ++i )
	{
	    var rec = recs[i];

	    if (! recidsSeen[rec.id])
		recidsSeen[rec.id] = 0;

	    recidsSeen[rec.id]++;
	}

	for ( i in recIds )
	{
	    var rid = recIds[i];
	    testlib.equal(1, recidsSeen[rid], "seen count for " + rid);
	}
    };

    this.testFilterOnLink = function() {
	var noSprintId = "TFOL001";
	if (! this.addBugWithId(noSprintId))
	    return;

	var withSprintId = "TFOL002";

        var repo = sg.open_repo(repInfo.repoName);
	var ztx = false;

	try
        {
            var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
            ztx = zs.begin_tx();

            var zrec = ztx.new_record("item");
	    zrec.title = "bug in sprint";
	    zrec.milestone = this.sprintId;
	    zrec.id = withSprintId;
            ztx.commit();
	}
	catch (e)
	{
	    testlib.fail(e.toString());

	    if (ztx)
		ztx.abort();

	    return;
	}
	finally
	{
		if (repo)
			repo.close();
	}

	var url = this.repoUrl + "/workitems.json?milestone=" + this.sprintId;

        var o = curl(
			"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
			(url)
		       );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

	var recsFound = {};

	var recs = JSON.parse(o.body);

	for ( var i = 0; i < recs.length; ++i )
	{
	    recsFound[recs[i].id] = true;
	}

	testlib.testResult(recsFound[withSprintId], 'expected ID ' + withSprintId);
	testlib.testResult(! recsFound[noSprintId], 'did not expect ID ' + noSprintId);
    };

    this.testFilterOnEmptyLink = function() {
		var noSprintId = "TFOEL001";
		if (! this.addBugWithId(noSprintId))
			return;

		var withSprintId = "TFOEL002";

        var repo = sg.open_repo(repInfo.repoName);
		var ztx = false;

		try
        {
            var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
            ztx = zs.begin_tx();

            var zrec = ztx.new_record("item");
			zrec.title = "bug in sprint";
			zrec.milestone = this.sprintId;
			zrec.id = withSprintId;
            ztx.commit();
		}
		catch (e)
		{
			testlib.fail(e.toString());

			if (ztx)
				ztx.abort();

			repo.close();

			return;
		}

		repo.close();
		var url = this.repoUrl + "/workitems.json?rectype=item&milestone=";

        var o = curl(
						"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
						(url)
					   );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

		var recsFound = {};

		var recs = JSON.parse(o.body);

		for ( var i = 0; i < recs.length; ++i )
		{
			recsFound[recs[i].id] = true;
		}

		testlib.testResult(recsFound[noSprintId], 'expected ID ' + withSprintId);
		testlib.testResult(! recsFound[withSprintId], 'did not expect ID ' + noSprintId);
    };

    this.testFilterOnEmptyField = function() {
	var noDescId = "TFOEnone0";
	if (! this.addBugWithId(noDescId))
	    return;

	var withDescId = "TFOEwith0";
	var emptyDescId = "TFOEempty0";

        var repo = sg.open_repo(repInfo.repoName);
	var ztx = false;

	try
        {
            var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
            ztx = zs.begin_tx();

            var zrec = ztx.new_record("item");
	    zrec.title = "bug with desc";
	    zrec.id = withDescId;
	    zrec.description = "I have a description";
            ztx.commit();

            ztx = zs.begin_tx();

            zrec = ztx.new_record("item");
	    zrec.title = "bug with empty desc";
	    zrec.id = emptyDescId;
	    zrec.description = "";
            ztx.commit();
	}
	catch (e)
	{
	    testlib.fail(e.toString());

	    if (ztx)
		ztx.abort();

	    repo.close();

	    return;
	}

	repo.close();
	var url = this.repoUrl + "/workitems.json?rectype=item&description=";

        var o = curl(
			"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
			(url)
		       );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

	var recsFound = {};

	var recs = JSON.parse(o.body);

	for ( var i = 0; i < recs.length; ++i )
	{
	    recsFound[recs[i].id] = true;
	}

	testlib.testResult(! recsFound[withDescId], 'did not expect ID ' + withDescId);
	testlib.testResult(recsFound[noDescId], 'expected ID ' + noDescId);
	testlib.testResult(recsFound[emptyDescId], 'expected ID ' + emptyDescId);
    };

    this.testFilterOnEmptyFieldAndOther = function() {
		var noDescId = "TFOEAOnone0";
		if (! this.addBugWithId(noDescId))
			return;

		var withDescId = "TFOEAOwith0";
		var emptyDescId = "TFOEAOempty0";

        var repo = sg.open_repo(repInfo.repoName);
		var ztx = false;

		try
        {
            var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
            ztx = zs.begin_tx();

            var zrec = ztx.new_record("item");
			zrec.title = "bug with desc";
			zrec.id = withDescId;
			zrec.description = "I have a description";
            ztx.commit();

            ztx = zs.begin_tx();

            zrec = ztx.new_record("item");
			zrec.title = "bug with empty desc";
			zrec.id = emptyDescId;
			zrec.description = "";
            ztx.commit();
		}
		catch (e)
		{
			testlib.fail(e.toString());

			if (ztx)
				ztx.abort();

			repo.close();

			return;
		}

		repo.close();
		var url = this.repoUrl + "/workitems.json?rectype=item&id=" + emptyDescId + "&description=";

        var o = curl(
						"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
						(url)
					   );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

		var recsFound = {};

		var recs = JSON.parse(o.body);

		for ( var i = 0; i < recs.length; ++i )
		{
			recsFound[recs[i].id] = true;
		}

		testlib.testResult(! recsFound[withDescId], 'did not expect ID ' + withDescId);
		testlib.testResult(! recsFound[noDescId], 'did not expect ID ' + noDescId);
		testlib.testResult(recsFound[emptyDescId], 'expected ID ' + emptyDescId);
    };

    this.testFilterOnMultiple = function() {
		var bugid = "TFOM001";
		var bugtitle = "bug " + bugid;
		if (! this.addBugWithId(bugid))
			return;

		var bugid2 = "TFOM002";
		if (! this.addBugWithId(bugid2, bugtitle))
			return;

		var url = this.repoUrl + "/workitems.json?id=" + bugid + "&title=" + bugtitle;

        var o = curl(
						"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
						(url)
					   );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

		var recsFound = {};

		var recs = JSON.parse(o.body);

		for ( var i = 0; i < recs.length; ++i )
		{
			recsFound[recs[i].id] = true;
		}

		testlib.testResult(recsFound[bugid], 'expected ID ' + bugid);
		testlib.testResult(! recsFound[bugid2], 'did not expect ID ' + bugid2);
    };


    this.testFilterOnEmpty = function() {
	var bugid = "TFOE001";
	var bugtitle = "bug " + bugid;
	if (! this.addBugWithId(bugid))
	    return;

	var url = this.repoUrl + "/workitems.json?rectype=item&assignee=";

        var o = curl(
			"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
			(url)
		       );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

	var recsFound = {};

	var recs = JSON.parse(o.body);

	for ( var i = 0; i < recs.length; ++i )
	{
	    recsFound[recs[i].id] = true;
	}

	testlib.testResult(recsFound[bugid], 'expected ID ' + bugid);
    };


/**
 *  re-enable these when contstraint-reporting is moved to SSJS
 */
    this.disabled_testViolateConstraints = function() {
        var url = this.repoUrl + "/wit/records/";
        var username = this.userId;
        var ourDesc = "This bug was added by the test testViolateConstraints.";

	var bugData = {
	    "rectype" : "item",
	    "id": this.bugSimpleId,  // known duplicate
	    "title" : ourDesc
	};
	var bugJson = JSON.stringify(bugData);

        var o = curl(
            "-H", "From: " + username,
            "-d", bugJson,
            (url));
        this.checkStatus("400 Bad Request", o.status, "posting");

	try
	{
	    var violations = JSON.parse(o.body);

	    if (testlib.testResult(!! violations.constraint_violations, "expected constraint_violations member"))
	    {
		var found = false;

		for ( var i = 0; i < violations.constraint_violations.length; ++i )
		{
		    var v = violations.constraint_violations[i];

		    if (v.type == "unique")
		    {
			found = true;
			break;
		    }
		}

		testlib.testResult(found, "expected 'unique' violation type");
	    }
	}
	catch ( e )
	{
	    testlib.fail("error: " + e.toString());
	}
    };

	this.disabled_testUpdateWorkItemConstraints = function() {
		var ourId = this.addBugWithId("tuwic001");

		var url = this.repoUrl + "/wit/records/" + ourId;
		var username = "testUpdateWorkItemConstraints <test@sourcegear.com>";

		var newData = {
			"id" : this.bugSimpleId
		};
		sg.file.write("tempUpdateWorkItemConstraints.json", JSON.stringify(newData));

		var o = curl(
		"-H", "From: " + username,
		"-T", "tempUpdateWorkItemConstraints.json",
		(url));
		this.checkStatus("400 Bad Request", o.status);

		try
		{
			var violations = JSON.parse(o.body);

			if (testlib.testResult(!! violations.constraint_violations, "expected constraint_violations member"))
			{
				var found = false;

				for ( var i = 0; i < violations.constraint_violations.length; ++i )
				{
					var v = violations.constraint_violations[i];

					if (v.type == "unique")
					{
						found = true;
						break;
					}
				}

		testlib.testResult(found, "expected 'unique' violation type");
	    }
	}
	catch ( e )
	{
	    testlib.fail("error: " + e.toString());
	}

	};

    this.disabled_testViolateConstraints = function() {
        var url = this.repoUrl + "/wit/records/";
        var username = this.userId;
        var ourDesc = "This bug was added by the test testViolateConstraints.";

	var bugData = {
	    "rectype" : "item",
	    "id": this.bugSimpleId,  // known duplicate
	    "title" : ourDesc
	};
	var bugJson = JSON.stringify(bugData);

        var o = curl(
            "-H", "From: " + username,
            "-d", bugJson,
            (url));
        this.checkStatus("400 Bad Request", o.status, "posting");

	try
	{
	    var violations = JSON.parse(o.body);

	    if (testlib.testResult(!! violations.constraint_violations, "expected constraint_violations member"))
	    {
		var found = false;

		for ( var i = 0; i < violations.constraint_violations.length; ++i )
		{
		    var v = violations.constraint_violations[i];

		    if (v.type == "unique")
		    {
			found = true;
			break;
		    }
		}

		testlib.testResult(found, "expected 'unique' violation type");
	    }
	}
	catch ( e )
	{
	    testlib.fail("error: " + e.toString());
	}
    };

    this.testSelectedFields = function() {
        var url = this.repoUrl + "/workitems.json?_fields=recid,id&rectype=item";

        var o = curl(
            "-H", "Accept: application/json",
            (url));
        this.checkStatus("200 OK", o.status, "retrieving records");

        var records = JSON.parse(o.body);

        if (!testlib.testResult((!!records) && (records.length > 0), "no records found"))
            return;


	var rec = records[0];

	testlib.testResult(!! rec.id, "expected id field");
	testlib.testResult(!! rec.recid, "expected recid field");
	testlib.testResult(! rec.title, "did not expect title field");
    };

	this.testAddBlocker = function()
	{
		var parentBugId = "TAB0001";
		var parentRecId = this.addBugWithId(parentBugId);

		if (! parentRecId)
			return;

        var url = this.repoUrl + "/workitem/" + this.bugId + "/blockeditems.json";

        var tname = "addBlockerJson.json";
        var username = this.userId;
		var data = {
			"linklist" :
			[
				{"recid" : parentRecId}
			]
		};

        sg.file.write(tname, JSON.stringify(data));

        var o = curl(
						"-H", "From: " + username,
						"-T", tname,
						(url));
        this.checkStatus("200 OK", o.status, "linking " + this.bugId + " to " + parentBugId, o.body);

		// parentRecId should have bugId in dependent list
		// bugId should have parentRecId in blocked list

        var repo = sg.open_repo(repInfo.repoName);
        var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
        var ztx = false;

		try
		{
			ztx = zs.begin_tx();

			var child = ztx.open_record('item', this.bugId);
			var parent = ztx.open_record(parentRecId);

			var blockedlist = child.blockeditems.to_array();
			var blockerlist = parent.dependents.to_array();

			var blocked = {};
			var blockers = {};

			for ( var i = 0; i < blockedlist.length; ++i )
			{
				blocked[blockedlist[i]] = true;
			}
			for ( var i = 0; i < blockerlist.length; ++i )
			{
				blockers[blockerlist[i]] = true;
			}

			testlib.testResult(!! blockers[this.bugId], "child expected in blocker list");
			testlib.testResult(!! blocked[parentRecId], "parent expected in blocked list");
		}
		catch (e)
		{
			print(e.toString());
		}

		if (ztx)
			ztx.abort();
		repo.close();
		};

	this.testMarkLinked = function()
	{
        var repo = sg.open_repo(repInfo.repoName);
		var cmtRecId = false;
		var ztx = false;

		try
		{
			var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			ztx = zs.begin_tx();

			var zrec = ztx.new_record("comment");
			cmtRecId = zrec.recid;
			zrec.text = "comment on " + this.bugSimpleId;
			zrec.item = this.bugId;
			ztx.commit();
		}
		catch (e)
		{
			testlib.fail(e.toString());

			if (ztx)
				ztx.abort();

			repo.close();

			return;
		}

		repo.close();

        var url = this.repoUrl + "/workitems.json?_includelinked=1&_fields=*,!";

        var o = curl(
						"-H", "From: " + this.userId,
                        "-H", "Accept: application/json",
						(url)
					   );

        if ((!testlib.assertSuccessfulExec(o)) ||
            (!this.checkStatus("200 OK", o.status)))
            return;

		var recs = JSON.parse(o.body);

        for ( var i = 0; i < recs.length; ++i )
		{
			var rec = recs[i];

			if (rec.rectype != 'item')
			{
				if (! testlib.testResult(rec._secondary, "non-items should be marked secondary"))
					break;
			}
		}

    };

	this._parseHeaders = function(headerBlock)
	{
        var s = headerBlock.replace(/[\r\n]+/g, "\n");
        var lines = s.split("\n");
		var headers = {};

		for ( var i = 0; i < lines.length; ++i )
		{
			var line = lines[i];

			var parts = line.split(':');

			if (parts.length == 2)
			{
				headers[parts[0]] = parts[1].replace(/^[ \t]+/, '');
			}
		}

		return(headers);
	}

    this.testFetchWorkItemCache = function() {
        var url = this.repoUrl + "/workitem/" +
        	this.bugId + ".json";
        var o = curl(
            (url));
        this.checkStatus("200 OK", o.status);

		var firstVersion = o.body;

		var firstHeaders = this._parseHeaders(o.headers);

		testlib.testResult(!! firstHeaders.ETag, "ETag header expected");
    };


    this.testActivityAtomCache = function() {
        var postUrl = this.repoUrl + "/activity";
        var listUrl = this.repoUrl + "/activity.xml";
        var username = this.userId;

		var o = curl(listUrl);
        this.checkStatus("200 OK", o.status, "retrieving first atom text at " + listUrl, o.body);

		var headers = this._parseHeaders(o.headers);

		if (! testlib.testResult(!! headers.ETag, "expected ETag header"))
			return;

		testlib.equal("application/atom+xml;charset=UTF-8", headers["Content-Type"], "content type");

		var etag = headers.ETag;

		var firstTime = o.body;

		o = curl( "-H", "If-None-Match: " + etag, (listUrl));
        this.checkStatus("304 Not Modified", o.status, "retrieving second atom text");
    };

	this.reviseBug = function(initialVals /* ... 1..n update val objects */)
	{
		if (! testlib.testResult(initialVals.id, "expected bug id"))
			return(false);

		var repo = sg.open_repo(repInfo.repoName);
		var ztx = false;

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			ztx = db.begin_tx(null, this.userId);
			var rec = ztx.new_record('item');
			var bugrecid = rec.recid;

			for ( var fn in initialVals )
				rec[fn] = initialVals[fn];

			var ct = ztx.commit();

			if (ct.errors)
			{
				testlib.fail(sg.to_json(ct));
			}

			ztx = false;

			for  ( var i = 1; i < arguments.length; ++i ) // not off-by-one
			{
				sleep(1100);

				var updateVals = arguments[i];

				ztx = db.begin_tx(null, this.userId);
				rec = ztx.open_record('item', bugrecid);

				for ( fn in updateVals )
					rec[fn] = updateVals[fn];

				ztx.commit();
				ztx = false;
			}

			repo.close();
			repo = false;
		}
		catch (e)
		{
			testlib.fail(e.toString());
			return(false);
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}

		return(bugrecid);
	};

	this.testKeepReorderOutOfActivity = function()
	{
		var bugid = "tKROOA";

		var bugrecid = this.reviseBug(
			{
				id: bugid,
				title: bugid,
				listorder: 1
			},
			{
				listorder: 2
			}
		);

		if (! bugrecid)
			return(false);

		// get activity
        var listUrl = this.repoUrl + "/activity.json";

		var activity = this.getTweets(listUrl).tweets;

		// this item should appear exactly once, with 'created' in the action
		var finds = [];

		for ( var i in activity )
		{
			var subject = activity[i].action;

			if (subject && (subject.indexOf(" " + bugid) > 0))
				finds.push(subject);
		}

		if (testlib.equal(1, finds.length, "activities found"))
			testlib.testResult(finds[0].indexOf('Created') == 0, "action should be 'Created'");
	};

	this.lastActivityFor  = function(recid)
	{
        var listUrl = this.repoUrl + "/activity.json";
		var activity = this.getTweets(listUrl).tweets;

		for ( var i in activity )
		{
			var act = activity[i];

			if (act.link.indexOf(recid) >= 0)
			{
				return(act);
			}
		}

		return(false);
	};

	this.testDescribeReopen = function()
	{
		var bugrec = this.reviseBug(
			{
				id: "tdrbug1",
				title: "tdrbug1",
				priority: "High",
				status: "fixed"
			},
			{
				status: "open"
			}
		);

		var act = this.lastActivityFor(bugrec);
		if (! testlib.testResult(!! act, "expected matching activity"))
			return;

		var expected = "Reopened tdrbug1";
		testlib.equal(expected, act.action, "action on reopened bug");
	};

	// assignee == assigned to ...
	this.testDescribeAssigned = function()
	{
		var bugrec = this.reviseBug(
			{
				id: "tdabug1",
				title: "tdabug1",
				priority: "High",
				status: "fixed"
			},
			{
				assignee: this.userId
			}
		);

		var act = this.lastActivityFor(bugrec);
		if (! testlib.testResult(!! act, "expected matching activity"))
			return;

		var expected = "Assigned tdabug1 to " + repInfo.userName;
		testlib.equal(expected, act.action, "action on assigned bug");
	};

	// verifier is now...
	this.testDescribeVerifier = function()
	{
		var bugrec = this.reviseBug(
			{
				id: "tdvbug1",
				title: "tdvbug1",
				priority: "High",
				status: "fixed"
			},
			{
				verifier: this.userId
			}
		);

		var act = this.lastActivityFor(bugrec);
		if (! testlib.testResult(!! act, "expected matching activity"))
			return;

		var expected = "verifier is now " + repInfo.userName;
		testlib.testResult(act.what.indexOf(expected) >= 0, "verifier note expected");
	};

	this.testSprintRelease = function()
	{
		var repo = sg.open_repo(repInfo.repoName);
		var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

		var url = this.repoUrl + "/milestone/release/" + this.sprintId + ".json";
		var username = this.userId;

		var releaseData =
		{
			rectype: "release",
			move: "false"
		};

		var releaseJson = JSON.stringify(releaseData);
		var o = curl( "-X", "PUT", "-H", "From: " + username, "-d", releaseJson,
				(url));
		this.checkStatus("200 OK", o.status, "posting", o.body);

		var released = zs.get_record('sprint', this.sprintId);

		testlib.ok(released.releasedate != undefined, "Sprint Released");
		repo.close();

	};

	this.testSprintReleaseWithMove = function()
	{
		var repo = sg.open_repo(repInfo.repoName);
		var ztx = null;

		try
		{
			var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

			ztx = zs.begin_tx();
			zrec = ztx.new_record("item");
			zrec.title = "Open bug to move";
			zrec.id = "openBugToMove";
			zrec.status = "open";
			var bug_id = zrec.recid;
			ztx.commit();

			ztx = zs.begin_tx();
			zrec = ztx.new_record("sprint");
			var prev_sprint_id = zrec.recid;
			zrec.description = "Sprint to release for stWeb tests.";
			zrec.name = "Prev Sprint";
			ztx.commit();

			ztx = zs.begin_tx();
			zrec = ztx.new_record("sprint");
			var next_sprint_id = zrec.recid;
			zrec.description = "Next sprint for stWeb tests.";
			zrec.name = "Next Sprint";
			ztx.commit();

			ztx = zs.begin_tx();
			var item = ztx.open_record('item', bug_id);
			item.milestone = prev_sprint_id;
			ztx.commit();
			ztx = null;

			var url = this.repoUrl + "/milestone/release/" + prev_sprint_id + ".json";
			var username = this.userId;

			var releaseData =
				{
					rectype: "release",
					move: "true",
					new_sprint: next_sprint_id
				};

			var releaseJson = JSON.stringify(releaseData);
			var o = curl( "-X", "PUT", "-H", "From: " + username, "-d", releaseJson,
							(url));
			this.checkStatus("200 OK", o.status, "posting", o.body);

			var released = zs.get_record('sprint', prev_sprint_id, ['*']);

			testlib.testResult(!! released.releasedate, "Sprint Released");

			var q = "milestone == '" + prev_sprint_id + "'";
			var releasedItems = this.query('item', q);

			if (releasedItems && releasedItems.length > 0)
			{
				for (var i = 0; i < releasedItems.length; i++)
				{
					item = releasedItems[i];
					if (item.status == "open")
					{
						testlib.fail("Open bug still in release sprint");
						return;
					}
				}
			}
			else
			{
				testlib.ok("Released Milestone has no items");

			}

			q = "milestone == '" + next_sprint_id + "'";
			var unreleasedItems = this.query('item', q);

			if ((! unreleasedItems) || (unreleasedItems.length == 0))
			{
				testlib.fail("No bugs in next sprint");
				return;
			}

			var moved = false;

			for (var i = 0; i < unreleasedItems.length; i++)
			{
				item = unreleasedItems[i];

				if (item.recid == bug_id)
				{
					moved = true;
				}
			}

			testlib.testResult(moved, "Open bug moved to new sprint?");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}
	};

	this.testSprintReleaseWithMoveSetCurrent = function()
	{
		var repo = sg.open_repo(repInfo.repoName);
		var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;

		try
		{
			ztx = zs.begin_tx();
			zrec = ztx.new_record("item");
			zrec.title = "Open bug to move";
			zrec.id = "openBugToMoveSC";
			zrec.status = "open";
			var bug_id = zrec.recid;
			ztx.commit();

			ztx = zs.begin_tx();
			zrec = ztx.new_record("sprint");
			var prev_sprint_id = zrec.recid;
			zrec.description = "Sprint to release for stWeb tests.";
			zrec.name = "Prev Sprint Set Current";
			ztx.commit();

			ztx = zs.begin_tx();
			zrec = ztx.new_record("sprint");
			var next_sprint_id = zrec.recid;
			zrec.description = "Next sprint for stWeb tests.";
			zrec.name = "Next Sprint St Current";
			ztx.commit();

			ztx = zs.begin_tx();
			var item = ztx.open_record('item', bug_id);
			item.milestone = prev_sprint_id;
			ztx.commit();

			ztx = null;

			var url = this.repoUrl + "/milestone/release/" + prev_sprint_id + ".json";
			var username = this.userId;

			var releaseData =
			{
				rectype: "release",
				move: "true",
				new_sprint: next_sprint_id,
				set_current: "1"
			};

			var releaseJson = JSON.stringify(releaseData);
			var o = curl( "-X", "PUT", "-H", "From: " + username, "-d", releaseJson,
					(url));
			this.checkStatus("200 OK", o.status, "posting", o.body);

			q = "milestone == '" + next_sprint_id + "'";
			var unreleasedItems = this.query('item', q);

			if ((! unreleasedItems) || (unreleasedItems.length == 0))
			{
				testlib.fail("No bugs in next sprint");
				return;
			}

			var moved = false;

			for (var i = 0; i < unreleasedItems.length; i++)
			{
				item = unreleasedItems[i];

				if (item.recid == bug_id)
				{
					moved = true;
				}
			}

			testlib.testResult(moved, "Open bug moved to new sprint?");

			var config = zs.query("config", ["*"], "key == 'current_sprint'", null, 1);

			if (!testlib.testResult(config.length > 0, "Config records found?"))
				return;

			var rec = config[0];

			testlib.equal(next_sprint_id, rec.value, "Next sprint set as current?");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if(repo)
				repo.close();
		}
	};

	this.testFetchConfig = function()
	{
		var url = this.repoUrl + "/config.json";
		o = curl(url);
		this.checkStatus("200 OK", o.status, "retrieving records");

		var records = false;
		records = JSON.parse(o.body);

		var found = false;

		if (!testlib.testResult((!!records) && (records.length > 0), "Records were found?"))
			return;

		for (var i = 0; i < records.length; ++i) {
			if (records[i].key == "testKey") {
				found = true;
				break;
			}
		}

		testlib.testResult(found, "Config record was found with matching key?");
	};

	this.testAddConfigRecord = function()
	{
		var url = this.repoUrl + "/config.json";
		var username = this.userId;

		var configData = {
			key: "newTestKey",
			value: "newTestValue"
		};
		var configJson = JSON.stringify(configData);
		var o = curl(
				"-H", "From: " + username,
				"-d", configJson,
				(url));
		this.checkStatus("200 OK", o.status, "posting");

		url = this.repoUrl + "/config.json";
		o = curl(
				"-H", "Accept: application/json",
				(url));
		this.checkStatus("200 OK", o.status, "retrieving records");

		var records = false;

		records = JSON.parse(o.body);

		if (!testlib.testResult((!!records) && (records.length > 0), "Records were found?"))
			return;

		var found = false;

		for (var i = 0; i < records.length; ++i) {

			if (records[i].key == configData.key) {
				found = true;
				break;
			}
		}

		testlib.testResult(found, "Config record was found with matching key?");
	};

	this.testAddConfigRecordUniqueFailure = function()
	{
		var url = this.repoUrl + "/config.json";
		var username = this.userId;

		var configData = {
			key: "testKey",
			value: "testValue"
		};
		var configJson = JSON.stringify(configData);
		var o = curl(
				"-H", "From: " + username,
				"-d", configJson,
				(url));

		this.checkStatus("400 Bad Request", o.status, "posting", o.body);
	};

	this.testUpdateConfigRecordByRecid = function()
	{
		var username = this.userId;
		var repo = sg.open_repo(repInfo.repoName);
		var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

		var url = this.repoUrl + "/config/" + this.configId;
		var updateData = {
			key: "testKey",
			value: "newValue"
		};

		var configJson = JSON.stringify(updateData);
		var o = curl(
				"-X", "PUT",
				"-H", "From: " + username,
				"-d", configJson,
				(url));
		this.checkStatus("200 OK", o.status, "posting", o.body);

		var rec = zs.get_record('config', this.configId);

		testlib.testResult(rec.key == "testKey", "Key wasn't updated?");
		testlib.testResult(rec.value == "newValue", "Value was updated?");

		repo.close();
	};

	this.testUpdateConfigRecordByKey = function()
	{
		var username = this.userId;
		var repo = sg.open_repo(repInfo.repoName);
		var zs = new zingdb(repo, sg.dagnum.WORK_ITEMS);

		try
		{
			var url = this.repoUrl + "/config/testKey";
			var updateData = {
				key: "testKey",
				value: "newNewValue"
			};

			var configJson = JSON.stringify(updateData);
			var o = curl(
					"-X", "PUT",
					"-H", "From: " + username,
					"-d", configJson,
					(url));
			this.checkStatus("200 OK", o.status, "posting", o.body);

			var rec = zs.get_record('config', this.configId);

			testlib.testResult(rec.key == "testKey", "Key wasn't updated?");
			testlib.testResult(rec.value == "newNewValue", "Value was updated?");

			repo.close();
			repo = null;
		}
		finally
		{
			if (repo)
				repo.close();
		}
	};

	// pull URI:
    // POST
    // body:


	this.testSingleHistory = function()
	{
		var bugid = this.addBugWithId("tsh1");

		var url = this.repoUrl + "/workitem/" + bugid + "/history.json";
		var username = this.userId;
		var testname = "testSingleHistory";

		var o = curl( "-H", "From: " + username,
						(url)
					   );

		var output = this.getJsonAndHeaders(url, "retrieving history");

		if (! output)
			return;

		var results = output.data;

		if (! testlib.ok(!! results, "non-empty results"))
			return;

		if (! testlib.equal(1, results.length, "result count"))
			return;

		var res = results[0];

		testlib.equal("bug tsh1", res.Name, "Name");
		testlib.equal(repInfo.userName, res.who, "who");
		testlib.equal("Created", res.action, "action");
	};

	this.testHistoryValues = function()
	{
		var bugid = this.addBugWithId("thv1");

		var repo = sg.open_repo(repInfo.repoName);
		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = db.begin_tx();
		var rec = ztx.open_record('item', bugid);
		rec.title = "new title";
		ztx.commit();
		repo.close();

		var url = this.repoUrl + "/workitem/" + bugid + "/history.json";
		var username = this.userId;
		var testname = "testHistoryValues";

		var o = curl( "-H", "From: " + username, url);

		if (! this.checkStatus("200 OK", o.status, "status retrieving history"))
	{
			testlib.log(o.body);
			return;
		}

		var results = JSON.parse(o.body);

		if (! testlib.ok(!! results, "non-empty results"))
			return;

		if (! testlib.equal(2, results.length, "result count"))
			return;

		var res = results[0];

		testlib.equal("new title", res.Name, "Name");
		testlib.equal(repInfo.userName, res.who, "who");
		testlib.equal("Modified", res.action, "action");

		res = results[1];

		testlib.equal("bug thv1", res.Name, "Name");
		testlib.equal(repInfo.userName, res.who, "who");
		testlib.equal("Created", res.action, "action");
	};

	this.testHistoryComments = function()
	{
		var bugid = this.addBugWithId("thc1");

		var repo = sg.open_repo(repInfo.repoName);
		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = db.begin_tx();

        var crec = ztx.new_record("comment");
	    var cmtRecId = crec.recid;
		crec.text = "comment added";
		crec.item = bugid;

		ztx.commit();

		repo.close();

		var url = this.repoUrl + "/workitem/" + bugid + "/history.json";
		var username = this.userId;
		var testname = "testHistoryValues";

		var o = curl( "-H", "From: " + username, url);

		if (! this.checkStatus("200 OK", o.status, "status retrieving history"))
		{
			testlib.log(o.body);
			return;
		}

		var results = JSON.parse(o.body);

		if (! testlib.ok(!! results, "non-empty results"))
			return;

		if (! testlib.equal(2, results.length, "result count"))
			return;

		var res = results[0];

		testlib.equal("comment added", res.comment, "Comment");
		testlib.equal(repInfo.userName, res.who, "who");
		testlib.equal("Comment", res.action, "action");

		res = results[1];

		testlib.equal("bug thc1", res.Name, "Name");
		testlib.equal(repInfo.userName, res.who, "who");
		testlib.equal("Created", res.action, "action");
	};

    this.testAttachmentAddHistory = function() {
		var bugid = this.addBugWithId("taah1");
		var testName = "testAttachmentAddHistory";

        var updateUrl = this.repoUrl + "/workitem/" + bugid + "/attachments";
        var username = this.userId;

		var fileData = "attachment from " + testName;
		var fn = testName + ".txt";
		sg.file.write(fn, fileData);

        var o = curl(
                        "-H", "From: " + username,
						"-X", "POST",
                        "-F", "file=@" + fn + ";filename=" + fn,
                        (updateUrl));

        if (!this.checkStatus("200 OK", o.status, "posting attachment"))
			return;

		var url = this.repoUrl + "/workitem/" + bugid + "/history.json";

		o = curl( "-H", "From: " + username, url);

		if (! this.checkStatus("200 OK", o.status, "status retrieving history"))
		{
			testlib.log(o.body);
			return;
		}

		var results = JSON.parse(o.body);

		if (! testlib.ok(!! results, "non-empty results"))
			return;

		if (! testlib.equal(2, results.length, "result count"))
			return;

		var res = results[0];

		testlib.ok(!! res.attachment, "attachment name");
		testlib.equal(repInfo.userName, res.who, "who");
		testlib.equal("File Attached", res.action, "action");

		res = results[1];

		testlib.equal("Created", res.action, "action");
	};

	this.testActivityClearedValue = function()
	{
		var bugid = "tACV1";

		var bugrecid = this.reviseBug(
			{
				id: bugid,
				title: bugid,
				description: "initial description",
				priority: "Medium",
				status: "open",
				assignee: this.userId,
				verifier: this.userId
			},
			{
				assignee: ""
			},
			{
				verifier: ""
			},
			{
				description: ""
			}
		);

		if (! bugrecid)
			return(false);

		// get activity
        var listUrl = this.repoUrl + "/activity.json";

		var activity = this.getTweets(listUrl).tweets;

		// this item should appear exactly once, with 'created' in the action
		var finds = [];

		for ( var i in activity )
		{
			var subject = activity[i].action;

			if (subject && (subject.indexOf(" " + bugid) > 0))
				finds.push(activity[i].what);
		}

		if (testlib.equal(4, finds.length, "activities found"))
		{
			testlib.testResult(finds[0].indexOf('description is now empty') >= 0, 'looking for "description is now empty"');
			testlib.testResult(finds[1].indexOf('verifier is now unassigned') >= 0, 'looking for "verifier is now unassigned"');
			testlib.testResult(finds[2].indexOf('Assigned to no one') >= 0, 'looking for "Assigned to no one"');
		}

		return(true);
	};

	this.testUnsetPriority = function()
	{
		var repo = sg.open_repo(repInfo.repoName);
		var ztx = null;

		try
		{
			var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
			ztx = db.begin_tx();
			var rec = ztx.new_record('item');
			var bugid = rec.recid;
			rec.title = "testUnsetPriority";
			rec.priority = "High";
			ztx.commit();
			ztx = false;

			var url = this.repoUrl + "/workitem/" + bugid + ".json";
			var fn = "testUnsetPriority.json";
			var upData = {
				priority: ""
			};
			var upJson = JSON.stringify(upData);

			sg.file.write(fn, upJson);

			var o = curl( "-H", "From: " + this.userId,
							"-T", fn,
							(url)
						   );
			if (!this.checkStatus("200 OK", o.status))
			{
				testlib.log(o.body);
				return;
			}

			rec = db.get_record('item', bugid);

			testlib.testResult(! rec.priority, "no priority expected");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}
	};

	this.testActivityOnlyMine = function()
	{
		var repo = sg.open_repo(repInfo.repoName);
		var userNameTwo = 'another@example.com';
		repo.create_user(userNameTwo);
		var udb = new zingdb(repo, sg.dagnum.USERS);
		var recs = udb.query('user', ['recid'], "name == '" + userNameTwo + "'");
		repo.close();

		if ((! recs) || (recs.length != 1))
			throw("failed to create / find user " + email);

		var userTwo = recs[0].recid;

		var bugid = "tAOM";

		var bugrecid = this.reviseBug(
			{
				id: bugid,
				title: bugid,
				description: "initial description",
				priority: "Medium",
				status: "open",
				assignee: this.userId,
				verifier: this.userId
			}
		);

		if (! bugrecid)
			return(false);

		// get activity
        var listUrl = this.repoUrl + "/activity/" + this.userId + ".json";

		var activity = this.getTweets(listUrl).tweets;

		// this item should appear exactly once, with 'created' in the action
		var finds = [];

		for ( var i in activity )
		{
			var subject = activity[i].action;

			if (subject && (subject.indexOf(" " + bugid) > 0))
				finds.push(activity[i].what);
		}

		testlib.ok(finds.length > 0, "expected in feed of creating user");

        listUrl = this.repoUrl + "/activity/" + userTwo + ".json";

		activity = this.getTweets(listUrl).tweets;

		finds = [];

		for ( i in activity )
		{
			var subject = activity[i].action;

			if (subject && (subject.indexOf(" " + bugid) > 0))
				finds.push(activity[i].what);
		}

		testlib.equal(0, finds.length, "not expected in feed of other user");
	};

	if (false)  // skip this test until H9882 is resolved
	this.testActivityCommentOnOldChangeset = function()
	{
		var tn = "testActivityCommentOnOldChangeset";
		for ( var i = 0; i < 50; ++i )
		{
			var fn = tn + i + ".txt";
            sg.file.write(fn, "stWeb");
            vscript_test_wc__add(fn);
            vscript_test_wc__commit();
		}

		var repo = sg.open_repo(repInfo.repoName);
		var ztx = null;

		try
		{
			var db = new zingdb(repo, sg.dagnum.VC_COMMENTS);
			ztx = db.begin_tx();
			var rec = ztx.new_record('item');
			rec.text = tn;
			rec.csid = this.changesetId;
			ztx.commit();
			ztx = false;

			var listUrl = this.repoUrl + "/activity.json";

			var tweets = this.getTweets(listUrl).tweets;
			var found = false;

			for (i = 0; i < tweets.length; ++i)
			{
				if (tweets[i].what.search(tn) >= 0)
				{
					found = true;
					break;
				}
			}

			testlib.testResult(found, "comment on old changeset found");
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}

	};

	this.testUpdateOutdatedWorkItem = function()
	{
		var oldTitle = "rev1 testUpdateOutdatedWorkItem title";
		var oldDesc = "rev1 testUpdateOutdatedWorkItem description";
		var newDesc = "rev2 testUpdateOutdatedWorkItem description";
		var newTitle = "rev3 testUpdateOutdatedWorkItem title";

		var bugid = this.reviseBug(
			{
				id: "tuowi1",
				title: oldTitle,
				description: oldDesc
			}
		);
		var getUrl = this.repoUrl + "/workitem/" + bugid + ".json";

		var results = this.getJsonAndHeaders(getUrl, "retrieve first rev");

		if (! results)
			return;

		var rev1 = results.data;

		var repo = sg.open_repo(repInfo.repoName);

		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;

		try
		{
			ztx = db.begin_tx(null, this.userId);
			var rec = ztx.open_record('item', bugid);
			rec.description = newDesc;
			ztx.commit();
			ztx = null;
		}
		catch (e)
		{
			testlib.fail(e.toString());
			return;
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}

		results = this.getJsonAndHeaders(getUrl, "retrieve second rev");

		if (! results)
			return;

		var rev2 = results.data;

		if (! testlib.testResult(rev1.description != rev2.description, "description should change"))
			return;

		if ((! testlib.testResult(!! rev1._csid, "csid expected on rev 1")) ||
			(! testlib.testResult(!! rev2._csid, "csid expected on rev 2")) ||
			(! testlib.testResult(rev1._csid != rev2._csid, "csid should change"))
		   )
			return;

		// update old rec via web. latest/greatest rec should have new title, and new description
		delete rev1.recid;
		delete rev1.rectype;

		var posturl = this.repoUrl + "/workitem/" + bugid + ".json";
		var fn = "tuowi.json";

		rev1.title = newTitle;

		testlib.log(JSON.stringify(rev1));

        sg.file.write(fn, JSON.stringify(rev1));

        var o = curl(
            "-H", "From: " + this.userId,
            "-T", fn,
            (posturl));

		testlib.log(o.body);

        if (!this.checkStatus("200 OK", o.status, "update status"))
		{
			return;
		}

		repo = sg.open_repo(repInfo.repoName);
		db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

		var rec = db.get_record('item', bugid);
		repo.close();

		testlib.equal(newTitle, rec.title, "updated title expected");
		testlib.equal(newDesc, rec.description, "updated description expected");
	};

	this.testWorkItemAutoComplete = function() {
		var tester = this;
		
		var expect = function(searchstr, expected) {
			var url = tester.repoUrl + "/workitem-fts.json?term=" + searchstr;

			var results = tester.getJsonAndHeaders(url, "fts on " + searchstr);

			if (! results)
				return(false);

			expected.sort();
			var got = results.data;
			var values = [];

			for ( var i = 0; i < got.length; ++i )
				values.push( got[i].value );

			values.sort();

			return( testlib.equal(JSON.stringify(expected), JSON.stringify(values), "fts on " + searchstr) );
		};

		var id1 = this.reviseBug(
			{
				id: "T_WIAC1",
				title: "firsttwiac TWIAC"
			}
		);
		var id2 = this.reviseBug(
			{
				id: "T_WIAC2",
				title: "secondtwiac TWIAC"
			}
		);

		expect("TWIAC", [id1, id2]);
		expect("T_WIAC", [id1, id2]);
		expect("T_WIAC1", [id1]);
		expect("secondtwiac", [id2]);
		expect("firsttwi", [id1]);
		expect("neither", []);
	};
	
	this.testAddWork = function()
	{
		var repo = sg.open_repo(repInfo.repoName);

		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;
		var now = new Date();
		
		try
		{
			ztx = db.begin_tx(null, this.userId);
			var rec = ztx.new_record('item');
			rec.title = "addWorkTest";
			var item_recid = rec.recid;
			ztx.commit();
			ztx = null;

		
			var url = this.repoUrl + "/workitem/" + item_recid + "/work.json";
			var username = this.userId;
			var workRec = {
				amount: 60,
				when: now.getTime()
			};
		
			var workJson = JSON.stringify(workRec);
			var o = curl( "-X", "PUT", "-H", "From: " + username, "-d", workJson,
					(url));
			if (!this.checkStatus("200 OK", o.status, "posting", o.body))
				return;
		
			var recs = db.query("work", ["*"], "item == '" + item_recid + "'");
			
			testlib.testResult(recs.length == 1, "Work record created", 1, recs.length);
		}
		catch (e)
		{
			testlib.fail(e.toString());
			return;
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}
	};
	
	this.testAddWorkWithEstimateUpdate = function()
	{
		var repo = sg.open_repo(repInfo.repoName);

		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;
		var now = new Date();
		
		try
		{
			ztx = db.begin_tx(null, this.userId);
			var rec = ztx.new_record('item');
			rec.title = "addWorkTest";
			var item_recid = rec.recid;
			ztx.commit();
			ztx = null;

		
			var url = this.repoUrl + "/workitem/" + item_recid + "/work.json";
			var username = this.userId;
			
			var workRec = {
				amount: 60,
				when: now.getTime(),
				estimate: 60
			}
		
			var workJson = JSON.stringify(workRec);
			var o = curl( "-X", "PUT", "-H", "From: " + username, "-d", workJson,
					(url));
			if (!this.checkStatus("200 OK", o.status, "posting", o.body))
				return;
		
			var recs = db.query("work", ["*"], "item == '" + item_recid + "'");
			
			testlib.testResult(recs.length == 1, "Work record created", 1, recs.length);
			
			var rec = db.get_record("item", item_recid);
			
			testlib.testResult(rec.estimate == 60, "Work record created", 60, rec.estimate);
		}
		catch (e)
		{
			testlib.fail(e.toString());
			return;
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}
	};
	
	this.testUpdateWorkitemWork = function()
	{
		var repo = sg.open_repo(repInfo.repoName);

		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
		var ztx = null;
		var now = new Date();
		
		try
		{
			ztx = db.begin_tx(null, this.userId);
			var zrec = ztx.new_record('item');
			zrec.title = "testUpdateWorkitemWork";
			zrec.estimate = 1;
			var item_recid = zrec.recid;
			ztx.commit();
			ztx = null;
			
			ztx = db.begin_tx(null, this.userId);
			var zrec = ztx.new_record('work');
			zrec.when = now.getTime()
			zrec.amount = 1;
			zrec.item = item_recid;
			var work_recid = zrec.recid;
			ztx.commit();
			ztx = null;
			
			ztx = db.begin_tx(null, this.userId);
			var zrec = ztx.new_record('work');
			zrec.when = now.getTime()
			zrec.amount = 1;
			zrec.item = item_recid;
			var deleted_work_recid = zrec.recid;
			ztx.commit();
			ztx = null;
			
			var data = {
				rectype: "item",
				recid: item_recid,
				estimate: 60,
				work: [
					{
						rectype: "work",
						recid: work_recid,
						amount: 30,
						when: now.getTime()
					},
					{
						rectype: "work",
						amount: 30,
						when: now.getTime()
					},
					{
						rectype: "work",
						recid: deleted_work_recid,
						_delete: true
					}
				]
			};
			
			var json = JSON.stringify(data);
			var url = this.repoUrl + "/workitem-full/" + item_recid + "/work.json";
			var username = this.userId;
			
			var o = curl( "-X", "PUT", "-H", "From: " + username, "-d", json,
					(url));
			
			if (!this.checkStatus("200 OK", o.status, "posting", o.body))
				return;
					
			var item_rec = db.get_record("item", item_recid);
			
			testlib.testResult(item_rec.estimate == 60, "Item estimate was updated", 60, item_rec.estimate);
			
			var work_recs = db.query("work", ["*"], "item == '" + item_recid + "'");
			
			testlib.testResult(work_recs.length == 2, "Correct number of associated work records", 2, work_recs.length);
			
			var deleted_rec = db.get_record("work", deleted_work_recid);
			
			testlib.testResult(!deleted_rec, "Work record was deleted");
			
			var updated_rec = db.get_record("work", work_recid);
			testlib.testResult(updated_rec.amount == 30, "Work record was updated", 30, updated_rec.amount);
			
			work = 0;
			for (var i = 0; i < work_recs.length; i++)
			{
				work += parseInt(work_recs[1].amount);
			}
			
			testlib.testResult(work == 60, "Correct amount of logged work", 60, work);
		}
		catch (e)
		{
			testlib.fail(e.toString());
			return;
		}
		finally
		{
			if (ztx)
				ztx.abort();
			if (repo)
				repo.close();
		}
	};

	this.testStaticNotModified = function() {
		var url = this.rootUrl + '/ui/sg.css';

        var o = curl(url);

		var h = o.headers;

        if (!this.checkStatus("200 OK", o.status, "first call status", o.body))
            return;

		var headers = {};

        var s = h.replace(/[\r\n]+/g, "\n");
        var lines = s.split("\n");

        for ( var i in lines )
        {
			var parts = lines[i].split(': ');

			headers[parts[0]] = parts[1];
		}
		
		var lastmod = headers["Last-Modified"];

		if (! testlib.ok(!! lastmod, "last-modified header expected"))
			return;

        o = curl( 
					'-H', 'If-Modified-Since: ' + lastmod,
					url);

        if (!this.checkStatus("304 Not Modified", o.status, "second call status", o.body))
            return;
    };

    this.testFileClasses = function ()
    {
        var o = curl(this.repoUrl + "/fileclasses.json");

        if (!this.checkStatus("200 OK", o.status, "first call status", o.body))
            return;

        var classes = JSON.parse(o.body);

        testlib.isNotNull(classes, "file classes should not be null");
    };

    this.testWitHovers = function ()
    {
        var url = this.repoUrl + "/workitems.json";
        var username = this.userId;
        var ourDesc = "This bug was added by the test testWitHovers.";
        var bugJson = "{"
            + "\"rectype\":\"item\","
            + "\"title\":\"" + ourDesc + "\"}";
        var o = curl(
            "-H", "From: " + username,
            "-d", bugJson,
            (url));
        if (!this.checkStatus("200 OK", o.status, "posting")) return;

        url = this.repoUrl + "/workitems.json";
        o = curl(
            "-H", "Accept: application/json",
            (url));
        if (!this.checkStatus("200 OK", o.status, "retrieving records")) return;

        var records = false;

        var records = JSON.parse(o.body);

        if (!testlib.testResult((!!records) && (records.length > 0), "no records found"))
            return;

        var found = false;

        for (var i = 0; i < records.length; ++i)
        {
            if (records[i].title == ourDesc)
            {
                found = records[i];
                break;
            }
        }
        print(found.recid);
        print(found.id);

        var o = curl(this.repoUrl + "/workitem/" + found.recid + "/hover.json");

        if (!this.checkStatus("200 OK", o.status, "first call status", o.body))
            return;

        var res = JSON.parse(o.body);

        testlib.isNotNull(res, "item should not be null");
        testlib.equal(res.title, "This bug was added by the test testWitHovers.", "title should be there");

        var o = curl(this.repoUrl + "/workitem/" + found.id + "/hover.json");

        if (!this.checkStatus("200 OK", o.status, "first call status", o.body))
            return;

        var res = JSON.parse(o.body);

        testlib.isNotNull(res, "item should not be null");
        testlib.equal(res.title, "This bug was added by the test testWitHovers.", "title should be there");
    };
    this.testGetChangeset = function ()
    {
        var o = curl(this.repoUrl + "/changesets/" + this.changeset3Id + ".json");

        if (!this.checkStatus("200 OK", o.status, "first call status", o.body))
            return;
        var res = JSON.parse(o.body);
        testlib.isNotNull(res, "changeset should not be null");

        o = curl(this.repoUrl + "/changesets/" + res.description.revno + ".json");
        if (!this.checkStatus("200 OK", o.status, "first call status", o.body))
            return;

        res = JSON.parse(o.body);
        testlib.isNotNull(res, "changeset should not be null");

    };

	this.testRejectInvalidAttachmentItem = function()
	{
        var updateUrl = this.repoUrl + "/workitem/undefined/attachments";
        var username = this.userId;

		var fileData = "\r\ntesting\r\n\r\n";
		var fn = "testRejectMissingAttachmentItem.txt";
		sg.file.write(fn, fileData);

        var o = curl(
                        "-H", "From: " + username,
						"-X", "POST",
                        "-F", "file=@" + fn,
                        updateUrl);

        if (! this.checkStatus("404 Not Found", o.status, "posting attachment with bad item", o.body))
			return;
	};

    if (this.serverHasWorkingCopy == false) this.testRepoRename = function ()
    {
        var oldRepoName = repInfo.repoName + "_testRepoRename";
        var newName = oldRepoName + "_renamed";
        sg.vv2.init_new_repo( { "repo" : oldRepoName, "no-wc" : true } );
        var repo = sg.open_repo(oldRepoName);
        var uid = repo.create_user("testRepoRename@sourcgear.com");

        var url = this.rootUrl + "/repos/manage/" + (oldRepoName) + ".json";

        var o = curl(
              "-H", "From: " + uid,
              "-X", "PUT",
		      "-d", JSON.stringify({ "name": newName }),
              url);

        if (!this.checkStatus("200 OK", o.status, "first call status", o.body))
            return;

        var repos = sg.list_descriptors();
        var found = false;
        var oldFound = false;
        for (var n in repos)
        {
            if (n == newName)
            {
                found = true;

            }
            if (n == oldRepoName)
            {
                oldFound = true;
            }
        }

        testlib.equal(found, true, "new repo name should exist in desriptors");
        testlib.equal(oldFound, false, "old repo name should not exist in descriptors");

        repo.close();
    };

	if(this.serverHasWorkingCopy) this.testDisallowedRemoteClone = function()
	{
		var my_destRepoHttpPath = this.rootUrl + "/repos/" + ("dest_" + sg.gid());

		var bCorrectFailure = false;
		try
		{
			sg.clone__exact(repInfo.repoName, my_destRepoHttpPath);
		}
		catch (e)
		{
			if (e.message.indexOf("Error 172") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
				bCorrectFailure = true;
			else
				print(sg.to_json__pretty_print(e));
		}
		testlib.testResult(bCorrectFailure, "Clone should fail with sglib error 172.");
	};

	if(this.serverHasWorkingCopy == false && testlib.defined.SG_CIRRUS === undefined) this.testSimpleRemoteClone = function()
	{
		var my_destName = "dest_" + sg.gid();
		var my_destRepoHttpPath = this.rootUrl + "/repos/" + (my_destName);

		sg.clone__exact(repInfo.repoName, my_destRepoHttpPath);

		var repo = sg.open_repo(repInfo.repoName);

		try
		{
			var bIdentical = repo.compare(my_destName);
			testlib.testResult(bIdentical, "After HTTP clone, repos are identical");
			var defaultPath = sg.local_settings().instance[repInfo.repoName].paths.default;
			print(defaultPath);
			testlib.equal(my_destRepoHttpPath, defaultPath, "After HTTP clone, destination repo should be the default sync path");
		}
		finally
		{
			repo.close();
		}
	};

}

function commitModFileCLC(name, stamp) {

    var folder = createFolderOnDisk(name);
    var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
    addRemoveAndCommit();
    insertInFile(file, 20);

    o = sg.exec(vv, "commit", "--message=fakemessage", "--stamp=" + stamp);
    testlib.ok(o.exit_status == 0, "exit status should be 0");

}
