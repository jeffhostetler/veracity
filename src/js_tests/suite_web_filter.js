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

function suite_web_filter()
{
    this.userId = "stewie@sourcegear.com";
    this.userId2 = "bosley@sourcegear.com";
    this.sprintId = "";
    this.sprintId2 = "";

    this.createLotsOfWitRecords = function (repo)
    {
        var itemidD = newRecord("item", repo, { title: "test depending"});
        var itemidB = newRecord("item", repo, { title: "test blocking"});
        var blocking = newRecord('item', repo, { title: 'blocking'});
        var blockrel = newRecord('relation',repo,
									  {
									      'relationship': 'blocks',
									      'source': blocking,
									      'target': itemidD
									  }
									 );

        var blocked = newRecord('item', repo, { title: 'blocked', status: 'open' });
        var blocksrel = newRecord('relation', repo, { relationship: 'blocks', 'source': itemidB, 'target': blocked});
      
        for (var i = 0; i < 5; i++)
        {
            var rec = {
                title: i.toString() + "Dear stupid dog, I've gone to live with the children on jolly farm. Good bye forever. Stewie.",
                status: "open",
                assignee: repInfo.userid,
                verifier: this.userId,
                priority: "High",
                estimate: 180
            };

            newRecord("item", repo, rec);
        }
        for (var i = 0; i < 5; i++)
        {
            var rec = {
                title: i.toString() + " What the deuce?",
                description: "Stewie: There's always been a lot of tension between Lois and me. And it's not so much that I want to kill her, it's just, I want her not to be alive anymore. " + i.toString(),
                status: "open",
                assignee: repInfo.userid,
                verifier: this.userId2,
                priority: "Low"

            };
            var itemid = newRecord("item", repo, rec);
            var commentid = newRecord("comment", this.repo, { text: "Stewie: Hello, mother. I come bearing a gift. I'll give you a hint. It's in my diaper and it's not a toaster. " + i.toString(), who: this.userId, item: itemid });
            if (!testlib.ok(!!commentid, "no comment id"))
                return;

        }
        for (var i = 0; i < 2; i++)
        {
            var nones = ["", "g3141592653589793238462643383279502884197169399375105820974944592" /*nobody*/];
            var rec = {
                title: i.toString() + " Stewie: You know, I rather like this God fellow.",
                description: " Very theatrical, you know. Pestilence here, a plague there. Omnipotence... gotta get me some of that. " + i.toString(),
                status: "fixed",
                assignee: nones[i],         
                verifier: nones[i],
                priority: "Low"
            };
            newRecord("item", repo, rec);

        }
        //no verifier or assignee
        var rec = {
            title: "Stewie: You know, I rather like this God fellow.",
            description: " Very theatrical, you know. Pestilence here, a plague there. Omnipotence... gotta get me some of that. " + i.toString(),
            status: "fixed",             
            priority: "Low"
        };
        newRecord("item", repo, rec);

        for (var i = 0; i < 100; i++)
        {
            var rec = {
                title: i.toString() + "Mother! I come bearing a gift",
                description: "Ill give you a hint: its in my diaper, and its not a toaster. " + i.toString(),
                status: "open",
                assignee: repInfo.userid,
                verifier: this.userId2,
                priority: "Medium",
                milestone: this.sprintId

            };

            var itemid = newRecord("item", repo, rec);

            if (!testlib.ok(!!itemid, "no item id"))
                return;

            var stampid = newRecord("stamp", repo, { name: "stewie", item: itemid });

            if (!testlib.ok(!!stampid, "no stamp id"))
                return;

            var stampid = newRecord("stamp", repo, { name: "deuce", item: itemid });

            if (!testlib.ok(!!stampid, "no stamp id"))
                return;
        }
     

    }
    this.suite_setUp = function ()
    {
        try
        {

            this.repo = sg.open_repo(repInfo.repoName);
            this.repo.create_user(this.userId);
            //  this.repo.create_user(this.userId2);

            this.repo.set_user(this.userId);

            // add a sprint
            var zs = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
            var ztx = zs.begin_tx();

            var zrec = ztx.new_record("sprint");
            this.sprintId = zrec.recid; // If you modify the bug, please leave the string "Common" in the description.
            this.sprintName = "mach1";
            zrec.description = "Common sprint for stFilter tests.";
            zrec.name = this.sprintName;
            ztx.commit();

            var serverFiles = sg.local_settings().machine.server.files;
            var core = serverFiles + "/core";
            load(serverFiles + "/dispatch.js");
            load(core + "/vv.js");

            var udb = new zingdb(this.repo, sg.dagnum.USERS);
            var urecs = udb.query('user', ['recid'], "name == '" + repInfo.userName + "'");

            repInfo.userid = urecs[0].recid;

            this.createLotsOfWitRecords(this.repo);
        }
        finally
        {
            if (this.repo)
                this.repo.close();
        }
    };

    this.testAddFilter = function ()
    {
        var filter = {
            "name": "testAddFilter",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "assignee": this.userId, "priority": "High" }
        };
        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recid = JSON.parse(o.body);
        print(recid);

        this.repo = sg.open_repo(repInfo.repoName);
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var savedFilter = db.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);
        this.repo.close();


        testlib.equal("testAddFilter", savedFilter.name);
        testlib.equal("priority", savedFilter.sort);
        testlib.equal("id,status,description", savedFilter.columns);
        testlib.equal("status", savedFilter.groupedby);
        testlib.equal(2, savedFilter.criteriatmp.length);


    };
    this.testUpdateFilter = function ()
    {
        //add a new filter
        var filter = {
            "name": "testUpdateFilter",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "status": "open" }
        };
        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recid = JSON.parse(o.body);
        print(recid);

        this.repo = sg.open_repo(repInfo.repoName);
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var savedFilter = db.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);
        this.repo.close();
        print(sg.to_json(savedFilter));

        testlib.equal("testUpdateFilter", savedFilter.name);
        testlib.equal("priority", savedFilter.sort);
        testlib.equal("id,status,description", savedFilter.columns);
        testlib.equal("status", savedFilter.groupedby);
        testlib.equal(1, savedFilter.criteriatmp.length);

        //add some new values 
        filter.criteria = { "status": "open,fixed", "priority": "High,Low" }
        filter.recid = recid;

        o = curl(
						"-H", "From: " + repInfo.userid,
						"-X", "PUT",
						"-d", JSON.stringify(filter),
						this.repoUrl + "/filter/" + recid + ".json");

        if (!testlib.equal("200 OK", o.status)) return;

        this.repo = sg.open_repo(repInfo.repoName);
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var savedFilter = db.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);
        this.repo.close();
        print(sg.to_json(savedFilter));

        testlib.equal(2, savedFilter.criteriatmp.length);
        for (var i = 0; i < savedFilter.criteriatmp.length; i++)
        {
            if (savedFilter.criteriatmp[i].fieldid == "status")
            {
                testlib.equal(savedFilter.criteriatmp[i].value, "open,fixed");
            }
            else if (savedFilter.criteriatmp[i].fieldid == "priority")
            {
                testlib.equal(savedFilter.criteriatmp[i].value, "High,Low");
            }
        }

        //remove a value in the same group
        filter.criteria = { "status": "fixed", "priority": "Low" }
        o = curl(
						"-H", "From: " + repInfo.userid,
						"-X", "PUT",
						"-d", JSON.stringify(filter),
						this.repoUrl + "/filter/" + recid + ".json");

        if (!testlib.equal("200 OK", o.status)) return;

        this.repo = sg.open_repo(repInfo.repoName);
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var savedFilter = db.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);
        this.repo.close();
        print(sg.to_json(savedFilter));

        testlib.equal(2, savedFilter.criteriatmp.length);
        for (var i = 0; i < savedFilter.criteriatmp.length; i++)
        {
            if (savedFilter.criteriatmp[i].fieldid == "status")
            {
                testlib.equal(savedFilter.criteriatmp[i].value, "fixed");
            }
            else if (savedFilter.criteriatmp[i].fieldid == "priority")
            {
                testlib.equal(savedFilter.criteriatmp[i].value, "Low");
            }
        }

        //remove all values in a group
        filter.criteria = { "status": "fixed" }
        o = curl(
						"-H", "From: " + repInfo.userid,
						"-X", "PUT",
						"-d", JSON.stringify(filter),
						this.repoUrl + "/filter/" + recid + ".json");

        if (!testlib.equal("200 OK", o.status)) return;

        this.repo = sg.open_repo(repInfo.repoName);
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var savedFilter = db.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);
        this.repo.close();
        print(sg.to_json(savedFilter));

        testlib.equal(1, savedFilter.criteriatmp.length);
        testlib.equal("fixed", savedFilter.criteriatmp[0].value);

        //remove all criteria
        filter.criteria = [];

        o = curl(
						"-H", "From: " + repInfo.userid,
						"-X", "PUT",
						"-d", JSON.stringify(filter),
						this.repoUrl + "/filter/" + recid + ".json");

        if (!testlib.equal("200 OK", o.status)) return;

        this.repo = sg.open_repo(repInfo.repoName);
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var savedFilter = db.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);
        this.repo.close();
        print(sg.to_json(savedFilter));

        testlib.ok(!savedFilter.criteriatmp);

        //change base filter values
        filter.sort = "status";
        filter.columns = "id,status,description,assignee";
        filter.groupedby = "assignee";

        o = curl(
						"-H", "From: " + repInfo.userid,
						"-X", "PUT",
						"-d", JSON.stringify(filter),
						this.repoUrl + "/filter/" + recid + ".json");

        if (!testlib.equal("200 OK", o.status)) return;

        this.repo = sg.open_repo(repInfo.repoName);
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var savedFilter = db.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);
        this.repo.close();
        print(sg.to_json(savedFilter));

        testlib.equal("status", savedFilter.sort);
        testlib.equal("id,status,description,assignee", savedFilter.columns);
        testlib.equal("assignee", savedFilter.groupedby);

    };
    this.testListFilters = function ()
    {
        var filter = {
            "name": "testListFilters",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "priority": "High" }
        };
        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recid = JSON.parse(o.body);

        url = this.repoUrl + "/filters.json?max=1";

        o = curl("-H", "From: " + repInfo.userid, url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body)
        testlib.equal(1, recs.length);

        url = this.repoUrl + "/filters.json?details";


        o = curl("-H", "From: " + repInfo.userid, url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body)
        testlib.ok(recs.length > 0);


    };
    this.testDeleteFilter = function ()
    {
        var filter = {
            "name": "testDeleteFilter",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "priority": "Medium", "verifier": this.userId2 }
        };
        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recid = JSON.parse(o.body);

        var url = this.repoUrl + "/filter/" + recid + ".json";
        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-X", "DELETE",
            "-d", recid,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        this.repo = sg.open_repo(repInfo.repoName);
        var zs = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var filters = zs.query("filter", ["*"], "recid == '" + recid + "'");

        testlib.equal(0, filters.length);

        var filtercrit = zs.query("filtercriteria", ["*"], "filter == '" + recid + "'");
        print(sg.to_json(filtercrit));
        testlib.equal(0, filtercrit.length);

        this.repo.close();

    };
    this.testFilterItems = function ()
    {
        var filter = {
            "name": "testFilterItemsHigh",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "priority": "High" }
        };
        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recidHigh = JSON.parse(o.body);

        var filter = {
            "name": "testFilterItemsCurrent",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "assignee": "currentuser" }
        };
        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recidCurrent = JSON.parse(o.body);

        var filter = {
            "name": "testFilterItemsEstimate",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "estimated": "yes" }
        };

        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recidEstimated = JSON.parse(o.body);

        var filter = {
            "name": "testFilterItemsNoEst",
            "user": repInfo.userName,
            "sort": "priority",
            "columns": "id,status,description",
            "groupedby": "status",
            "criteria": { "estimated": "no" }
        };

        var url = this.repoUrl + "/filters.json";
        var j = JSON.stringify(filter);

        var o = curl(
            "-H", "From: " + repInfo.userid,
            "-d", j,
            url);
        if (!testlib.equal("200 OK", o.status)) return;

        var recidNotEstimated = JSON.parse(o.body);

        url = this.repoUrl + "/filteredworkitems.json?skip=0&maxrows=100&milestone=" + this.sprintId + "&assignee=currentuser";

        o = curl("-H", "From: " + repInfo.userid, url);
        var recs = JSON.parse(o.body)
        testlib.equal(100, recs.results.length);

        var json = {
            filter: recidHigh,
            skip: 0,
            maxrows: 100
        };


        url = this.repoUrl + "/filteredworkitems.json?filter=" + recidHigh;

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body)
        testlib.equal(5, recs.results.length);

        json = {
            //  filter: recidCurrent,
            skip: 0,
            maxrows: 100
        };

        url = this.repoUrl + "/filteredworkitems.json?filter=" + recidCurrent;
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body)
        testlib.equal(100, recs.results.length);


        json = {
            //  filter: recidCurrent,
            skip: 0,
            stamp: "stewie",
            maxrows: 150,
            priority: "(none),High,Medium,Low",
            status: "open,fixed"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body)
        testlib.equal(100, recs.results.length);

        json = {
            //  filter: recidCurrent,
            skip: 0,
            stamp: "stewie",
            maxrows: 150,
            priority: "(none),High,Medium,Low",
            status: "open,fixed",
            assignee: "currentuser"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body)
        testlib.equal(100, recs.results.length);

        json = {
            //  filter: recidCurrent,
            skip: 0,
            stamp: "stewie",
            maxrows: 150,
            milestone: this.sprintId,
            priority: "(none),High,Medium,Low",
            status: "open,fixed",
            assignee: "currentuser"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body)
        testlib.equal(100, recs.results.length);

        json = {
            //  filter: recidCurrent,
            skip: 0,
            stamp: "stewie,deuce",
            maxrows: 150,
            milestone: this.sprintId,
            priority: "(none),High,Medium,Low",
            status: "open,fixed",
            assignee: "currentuser"
        };
        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body)
        testlib.equal(100, recs.results.length);

        json = {
            //  filter: recidCurrent,         
            stamp: "stewie"

        };
        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body)
        testlib.equal(100, recs.results.length);

        json = {
            //  filter: recidCurrent,         
            estimated: "yes"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(5, recs.results.length);

        json = {
            //  filter: recidCurrent,         
            estimated: "no"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(112, recs.results.length);


        json = {
            verifier: "(none)",
            assignee: "(none)"
        };
        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(7, recs.results.length);

        json = {
            filter: recidEstimated

        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(5, recs.results.length);

        json = {
            filter: recidNotEstimated
        };
        url = this.repoUrl + "/filteredworkitems.json?filter=" + recidNotEstimated + "&skip=0&maxrows=110";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(112, recs.results.length);


        json = {
            keyword: "stupid",
            status: "open"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(5, recs.results.length);

        json = {
            keyword: "toaster"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(105, recs.results.length);

        json = {
            blocking: "true"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(2, recs.results.length);

        json = {
            depending: "true"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(2, recs.results.length);

        json = {
            depending: "true",
            blocking: "true"
        };

        url = this.repoUrl + "/filteredworkitems.json";
        testlib.log("url: " + url);

        o = curl("-H", "From: " + repInfo.userid, "-d", JSON.stringify(json), url);
        var recs = JSON.parse(o.body);
        testlib.equal(4, recs.results.length);

        // remaining_time = Estimated
        //todo current milestone
    };
}
