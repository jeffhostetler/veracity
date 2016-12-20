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

function suite_web_search()
{
    this.lotsCreated = false;
    this.lotsWikiCreated = false;
    this.seriesRecId;
    this.statusRecId;
    this.environmentRecId;
  
    this.createLotsOfVCRecords = function ()
    {
        this.repo = sg.open_repo(repInfo.repoName);
        //add changeset stuff for search
        for (var i = 0; i < 35; i++)
        {
            var folder = createFolderOnDisk("st_search_test_vcfolder" + i);
            var file = createFileOnDisk(pathCombine(folder, "file" + i), 39);
            vscript_test_wc__addremove();
            var csid = vscript_test_wc__commit();
            if (!testlib.ok(!!this.csid, "no csid"))
                return;

            audit = this.repo.create_audit(this.userId);
            this.repo.add_vc_comment(csid, i +" I do mind, the Dude minds. This will not stand, ya know, this aggression will not stand, man.", audit);
            audit.fin();
        }
        if (this.repo)
            this.repo.close();
    }
    this.createLotsOfBuilds = function ()
    {
        this.repo = sg.open_repo(repInfo.repoName);
        var zdb = new zingdb(this.repo, sg.dagnum.BUILDS);

        try
        {
            for (var i = 0; i < 20; i++)
            {
                var r;
                try
                {
                    var ztx = zdb.begin_tx();
                    var buildNameRec = ztx.new_record("buildname");
                    var buildRec = ztx.new_record("build");
                    buildNameRec.name = "searchTestBuild_" + i;
                    buildNameRec.csid = sg.vv2.leaves()[0];
                    buildNameRec.series_ref = this.seriesRecId;

                    print(sg.to_json__pretty_print(buildNameRec));

                    buildRec.buildname_ref = buildNameRec.recid;
                    buildRec.environment_ref = this.environmentRecId;
                    buildRec.series_ref = this.seriesRecId;
                    buildRec.status_ref = this.statusRecId;

                    r = ztx.commit();
                }
                finally
                {
                    if (r != null && r.errors != null)
                    {
                        ztx.abort();
                        testlib.ok(false, sg.to_json__pretty_print(r));
                    }
                    else
                        testlib.ok(true, "Basic build added.");
                }
            }
        }
        finally
        {

            if (this.repo)
                this.repo.close();
        }
    }
    this.createLotsOfWikiRecords = function ()
    {
        this.lotsWikiCreated = true;
        this.repo = sg.open_repo(repInfo.repoName);
        for (var i = 0; i < 15; i++)
        {
            var db = new zingdb(this.repo, sg.dagnum.WIKI);
            var ztx = db.begin_tx(null, this.userId);
            var newrec = ztx.new_record('page');
            newrec.title = "wiki title " + i;
            newrec.text = 'wiki page ' + i;
            vv.txcommit(ztx);
        }
        if (this.repo)
            this.repo.close();
    }
    this.createLotsOfWitRecords = function ()
    {
        this.lotsCreated = true;
        this.repo = sg.open_repo(repInfo.repoName);
        for (var i = 0; i < 15; i++)
        {
            var rec = {
                title: i.toString() + "Dear stupid dog, I've gone to live with the children on jolly farm. Good bye forever. Stewie.",
                status: "open"
            };

            newRecord("item", this.repo, rec);
        }
        for (var i = 0; i < 15; i++)
        {
            var rec = {
                title: i.toString() + " What the deuce?",
                description: "Stewie: There's always been a lot of tension between Lois and me. And it's not so much that I want to kill her, it's just, I want her not to be alive anymore. " + i.toString(),
                status: "open"
            };
            newRecord("item", this.repo, rec);

        }
        for (var i = 0; i < 15; i++)
        {
            var rec = {
                title: i.toString() + "boring title",

                status: "open"
            };

            // var rec = { title: "What do you do for recreation?", description: "Oh, the usual. I bowl. Drive around. The occasional acid flashback.", comment: "Also, my rug was stolen." };
            var itemid = newRecord("item", this.repo, rec);

            if (!testlib.ok(!!itemid, "no item id"))
                return;

            var commentid = newRecord("comment", this.repo, { text: "Stewie: Hello, mother. I come bearing a gift. I'll give you a hint. It's in my diaper and it's not a toaster. " + i.toString(), who: this.userId, item: itemid });
            if (!testlib.ok(!!commentid, "no comment id"))
                return;

        }
        for (var i = 0; i < 15; i++)
        {
            var rec = {
                title: i.toString() + "another boring title",
                status: "open"
            };

            // var rec = { title: "What do you do for recreation?", description: "Oh, the usual. I bowl. Drive around. The occasional acid flashback.", comment: "Also, my rug was stolen." };
            var itemid = newRecord("item", this.repo, rec);

            if (!testlib.ok(!!itemid, "no item id"))
                return;

            var stampid = newRecord("stamp", this.repo, { name: "stewie" + i, item: itemid });

            if (!testlib.ok(!!stampid, "no stamp id"))
                return;
        }

        if (this.repo)
            this.repo.close();
    }
    this.suite_setUp = function ()
    {
        try
        {
            this.userId = "test@sourcegear.com";
            this.repo = sg.open_repo(repInfo.repoName);
            this.repo.create_user(this.userId);
            this.repo.set_user(this.userId);

            var serverFiles = sg.local_settings().machine.server.files;
            var core = serverFiles + "/core";
            load(serverFiles + "/dispatch.js");
            load(core + "/vv.js");

            var rec = {
                title: "What do you do for recreation?",
                description: "Oh, the usual. I bowl. Drive around. The occasional acid flashback.",
                status: "open"
            };

            // var rec = { title: "What do you do for recreation?", description: "Oh, the usual. I bowl. Drive around. The occasional acid flashback.", comment: "Also, my rug was stolen." };
            var itemid = newRecord("item", this.repo, rec);
            this.itemid = itemid;
            if (!testlib.ok(!!itemid, "no item id"))
                return;

            var commentid = newRecord("comment", this.repo, { text: "Also, my rug was stolen.", who: this.userId, item: itemid });
            if (!testlib.ok(!!commentid, "no comment id"))
                return;

            var stampid = newRecord("stamp", this.repo, { name: "dude", item: itemid });

            if (!testlib.ok(!!stampid, "no stamp id"))
                return;

            //add changeset stuff for search
            var folder = createFolderOnDisk("st_search_test_folder");
            var file = createFileOnDisk(pathCombine(folder, "file1.txt"), 39);
            vscript_test_wc__addremove();
            this.csid = vscript_test_wc__commit();
            if (!testlib.ok(!!this.csid, "no csid"))
                return;

            var audit = this.repo.create_audit(this.userId);
            this.repo.add_vc_stamp(this.csid, "Nihilists", audit);
            audit.fin();

            audit = this.repo.create_audit(this.userId);
            this.repo.add_vc_tag(this.csid, "Walter", audit);
            audit.fin();

            audit = this.repo.create_audit(this.userId);
            this.repo.add_vc_comment(this.csid, "You want a toe? I can get you a toe, believe me. There are ways, Dude. You don't wanna know about it, believe me.", audit);
            audit.fin();

            // add a build
            var zdb = new zingdb(this.repo, sg.dagnum.BUILDS);
            var ztx = zdb.begin_tx();

            var zrec = ztx.new_record("environment");
            this.environmentRecId = zrec.recid;
            zrec.name = "search test build";
            zrec.nickname = "stb";

            zrec = ztx.new_record("series");
            this.seriesRecId = zrec.recid;
            zrec.name = "Continuous";
            zrec.nickname = "C";


            zrec = ztx.new_record("status");
            this.statusRecId = zrec.recid;
            zrec.name = "Success";
            zrec.nickname = "S";
            zrec.color = "green";
            zrec.temporary = false;
            zrec.successful = true;


            var r = ztx.commit();


            var ztx = zdb.begin_tx();
            var buildNameRec = ztx.new_record("buildname");
            var buildRec = ztx.new_record("build");

            try
            {
                buildNameRec.name = "0.11.11.1970";
                buildNameRec.csid = sg.vv2.leaves()[0];
                buildNameRec.series_ref = this.seriesRecId;

                print(sg.to_json__pretty_print(buildNameRec));

                buildRec.buildname_ref = buildNameRec.recid;
                buildRec.environment_ref = this.environmentRecId;
                buildRec.series_ref = this.seriesRecId;
                buildRec.status_ref = this.statusRecId;

                r = ztx.commit();
            }
            finally
            {
                if (r != null && r.errors != null)
                {
                    ztx.abort();
                    testlib.ok(false, sg.to_json__pretty_print(r));
                }
                else
                    testlib.ok(true, "Basic build added.");
            }
            if (r.errors != null)
            {
                ztx.abort();
                testlib.ok(false, sg.to_json__pretty_print(r));
            }
            else
                testlib.ok(true, "Basic build link data added.");

        }

        finally
        {
            if (this.repo)
                this.repo.close();
        }
        this.createLotsOfWikiRecords();
    };


    this.testSearchWitAdv = function ()
    {
        var url = this.repoUrl + "/searchadvanced.json?text=recreation&areas=wit&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        print(o.body);
        var recs = JSON.parse(o.body);
        testlib.ok(recs.wit.items.length == 1, "should be one itemid match");
        testlib.ok(recs.wit.items[0].matcharea == "itemFields", "match area should be itemFeilds");

        var url = this.repoUrl + "/searchadvanced.json?text=bowl&areas=wit&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.wit.items.length == 1, "should be one itemid match");
        testlib.ok(recs.wit.items[0].matcharea == "itemFields", "match area should be itemFeilds");

        var url = this.repoUrl + "/searchadvanced.json?text=rug&areas=wit&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.wit.items.length == 1, "should be one itemid match");
        testlib.ok(recs.wit.items[0].matcharea == "comment", "match area should be comment");


        var url = this.repoUrl + "/searchadvanced.json?text=dude&areas=wit&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.wit.items.length == 1, "should be one itemid match");
        testlib.ok(recs.wit.items[0].matcharea == "stamp", "match area should be comment");

    }

    this.testSearchWikiAdv = function ()
    {
    
        var url = this.repoUrl + "/searchadvanced.json?text=wiki&areas=wiki&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        print(o.body);
        var recs = JSON.parse(o.body);
        testlib.ok(recs.wiki.items.length == 15, "should be 15 wiki matches");

    }

    this.testSearchVCAdv = function ()
    {
        var url = this.repoUrl + "/searchadvanced.json?text=" + this.csid.slice(0, 5) + "&areas=vc&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
      
        var recs = JSON.parse(o.body);
        testlib.ok(recs.vc.items.length >= 1, "should be at least one vc match");
        testlib.ok(recs.vc.items[0].matcharea == "id", "match area should be id");

        var url = this.repoUrl + "/searchadvanced.json?text=toe&areas=vc&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.vc.items.length == 1, "should be one vc match");
        testlib.ok(recs.vc.items[0].matcharea == "comment", "match area should be comment");


        var url = this.repoUrl + "/searchadvanced.json?text=Nihilists&areas=vc&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.vc.items.length == 1, "should be one vc match");
        testlib.ok(recs.vc.items[0].matcharea == "stamp", "match area should be stamp");


        var url = this.repoUrl + "/searchadvanced.json?text=Walter&areas=vc&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.vc.items.length == 1, "should be one vc match");
        testlib.ok(recs.vc.items[0].matcharea == "tag", "match area should be tag");

    }

    this.testSearchBuildsAdv = function ()
    {       
        var url = this.repoUrl + "/searchadvanced.json?text=1970&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.builds.items.length == 1, "should be one build match");
        testlib.ok(recs.builds.items[0].matcharea == "name", "match area should be name");

    }

    this.textSearchErrors = function ()
    {
        if (!this.lotsCreated)
        {
            this.createLotsOfWitRecords();
        }
        var url = this.repoUrl + "/searchadvanced.json?text=(stewie&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.error, "should be error");

        var url = this.repoUrl + "/searchadvanced.json?text=(ste)wie)&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.error, "should be error");

        var url = this.repoUrl + "/searchadvanced.json?text=\"stewie&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.error, "should be error");
       

        var url = this.repoUrl + "/searchresults.json?text=(stewie&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.items.length > 0, "should not be be error");

        var url = this.repoUrl + "/searchresults.json?text=\"stewie&max=100";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.items.length > 0, "should not be be error");

    }
    
    this.testSearchWitPaging = function ()
    {
        if (!this.lotsCreated)
        {
            this.createLotsOfWitRecords();
        }
        //first test just basic search results used for quick search
        var url = this.repoUrl + "/searchresults.json?text=stewie";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(recs.items.length, 15, "should be 15 results");
        testlib.equal(recs.more, true, "should be more results");

        var url = this.repoUrl + "/searchresults.json?text=%22stew%22";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(recs.items.length, 0, "should be 0 results");

        var url = this.repoUrl + "/searchresults.json?text=%22stewie+dog%22";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(recs.items.length, 0, "should be 0 results");


        var url = this.repoUrl + "/searchresults.json?text=stewie+dog";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.ok(recs.items.length > 0, "should be results");

        var allItems = [];
        var more = true;

        var matchCount = {};
        var lastField = "";

        while (more == true)
        {
            var url = this.repoUrl + "/searchadvanced.json?text=stewie&areas=wit&max=20&skip=" + matchCount[lastField] + "&lf=" + lastField;

            o = curl(url);
            if (!testlib.equal("200 OK", o.status)) return;

            var recs = JSON.parse(o.body);
            var items = recs.wit.items;
            for (var i = 0; i < items.length; i++)
            {
                allItems.push(items[i]);
                lastField = items[i].matcharea;
                if (!matchCount[lastField])
                {
                    matchCount[lastField] = 0;
                }
                matchCount[lastField]++;
            }

            more = recs.wit.more;

        }

        testlib.equal(allItems.length, 60, "should be 60 records");
    }


    this.testSearchVCPaging = function ()
    {
        this.createLotsOfVCRecords();

        //first test just basic search results used for quick search
        var url = this.repoUrl + "/searchresults.json?text=aggression&max=20";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(20, recs.items.length, "should be 20 results");
        testlib.equal(recs.more, true, "should be more results");

        var url = this.repoUrl + "/searchresults.json?text=aggression&max=40";
        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(35, recs.items.length, "should be 35 results");
        testlib.equal(recs.more, false, "should not be more results");

        var allItems = [];
        var more = true;

        var matchCount = {};
        var lastField = "";

        while (more == true)
        {
            var url = this.repoUrl + "/searchadvanced.json?text=aggression&areas=vc&max=15&skip=" + matchCount[lastField] + "&lf=" + lastField;

            o = curl(url);
            if (!testlib.equal("200 OK", o.status)) return;

            var recs = JSON.parse(o.body);
            print(recs);
            var items = recs.vc.items;
            for (var i = 0; i < items.length; i++)
            {
                allItems.push(items[i]);
                lastField = items[i].matcharea;
                if (!matchCount[lastField])
                {
                    matchCount[lastField] = 0;
                }
                matchCount[lastField]++;
            }

            more = recs.vc.more;

        }

        testlib.equal(35, allItems.length, "should be 35 records");

    }


    this.testSearchBuildPaging = function ()
    {
        this.createLotsOfBuilds();
        //first test just basic search results used for quick search
        var url = this.repoUrl + "/searchresults.json?text=searchTestBuild&max=5";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(5, recs.items.length, "should be 5 results");
        testlib.equal(recs.more, true, "should be more results");

        var url = this.repoUrl + "/searchresults.json?text=searchTestBuild&max=30";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(20, recs.items.length, "should be 20 results");
        testlib.equal(recs.more, false, "should not be more results");

       

        var allItems = [];
        var more = true;

        var matchCount = {};
        var lastField = "";

        while (more == true)
        {
            var url = this.repoUrl + "/searchadvanced.json?text=searchTestBuild&areas=builds&max=7&skip=" + matchCount[lastField] + "&lf=" + lastField;

            o = curl(url);
            if (!testlib.equal("200 OK", o.status)) return;

            var recs = JSON.parse(o.body);
            var items = recs.builds.items;
            for (var i = 0; i < items.length; i++)
            {
                allItems.push(items[i]);
                lastField = items[i].matcharea;
                if (!matchCount[lastField])
                {
                    matchCount[lastField] = 0;
                }
                matchCount[lastField]++;
            }

            more = recs.builds.more;

        }

        testlib.equal(20, allItems.length, "should be 20 records");

    }

    this.testSearchWikiPaging = function ()
    {
       
        //first test just basic search results used for quick search
        var url = this.repoUrl + "/searchresults.json?text=wiki&max=5";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(5, recs.items.length, "should be 5 results");
        testlib.equal(recs.more, true, "should be more results");

        var url = this.repoUrl + "/searchresults.json?text=wiki&max=30";

        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        testlib.equal(15, recs.items.length, "should be 15 results");
        testlib.equal(recs.more, false, "should not be more results");



        var allItems = [];
        var more = true;

        var matchCount = {};
        

        while (more == true)
        {
            var url = this.repoUrl + "/searchadvanced.json?text=wiki&areas=wiki&max=7&skip=" + matchCount["wiki"] + "&lf=wiki";

            o = curl(url);
            if (!testlib.equal("200 OK", o.status)) return;

            var recs = JSON.parse(o.body);
            var items = recs.wiki.items;
            for (var i = 0; i < items.length; i++)
            {
                allItems.push(items[i]);
                
                if (!matchCount["wiki"])
                {
                    matchCount["wiki"] = 0;
                }
                matchCount["wiki"]++;
            }

            more = recs.wiki.more;

        }

        testlib.equal(15, allItems.length, "should be 15 records");

    }


    this.testSearchAll = function ()
    {
        if (!this.lotsCreated)
        {
            this.createLotsOfWitRecords();
        }
        var url = this.repoUrl + "/searchadvanced.json?text=stewie&areas=wit&max=65&skip=0";
        o = curl(url);
        if (!testlib.equal("200 OK", o.status)) return;
        var recs = JSON.parse(o.body);
        
        testlib.equal(recs.wit.items.length, 60, "should be 60 results");
        testlib.equal(recs.wit.more, false, "should be no more results");
    }

}
