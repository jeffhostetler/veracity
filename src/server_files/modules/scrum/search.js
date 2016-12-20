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

function getSearchText(term)
{
    // term = term.replace(/'/g, "''");
    term = term.trim();

    if (term.indexOf('"') != 0 && term.indexOf(" ") < 0)
    {
        term += "*";
    }

    return term;
}

function searchRequest(repo)
{
    this.repo = repo;
    this.userHash = getUserHash(repo);
    var self = this;

    function createSearchRecordItem(record, details, matcharea)
    {
        var item = {
            name: record.id,
            id: record.recid,
            extra: record.status || "",
            description: record.title,
            category: "work items",
            matcharea: matcharea

        }
        if (details)
        {
            item.matchtext = record["description"] || record["text"] || ""; //don't always have a description
        }
        return item;
    }

    function searchWitJoins(db, rectype, searchfield, term, numToFetch, start, details)
    {      
        // var recs = db.query__fts(rectype, searchfield, term, fields, numToFetch + 1, start);
        var searchRecs = db.query__fts(rectype, searchfield, term, ["item", searchfield], numToFetch + 1, start);
     
        var recids = [];

        for (var i = 0; i < searchRecs.length; i++)
        {
            recids.push(searchRecs[i].item);
        }
        var fields = ["recid", "id", "status", "title"];

        if (details)
        {
            fields.push("description");
        }
        var recs = db.q({
            "rectype": "item",
            "fields": fields,
            "where": ["recid", "in", recids]
        });

        return recs;
    }

    this.doItemFullTextSearch = function (text, max, details, skip, lastField, restart)
    {
        var fields = ["itemFields", "comment", "stamp"];
        var results = [];
        var items = [];
        var more = false;
        max = parseInt(max);
        start = skip || 0;
        var term = getSearchText(text);
        var index = 0;
        var done = false;


        for (var i = 0; i < fields.length; i++)
        {
            if (fields[i] == lastField)
            {
                index = i;
            }
        }
        var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
        var self = this;
        for (var i = index; i < fields.length; i++)
        {
            if (fields[i] == fields[fields.length - 1])
                done = true;

            if (!max || items.length <= max)
            {
                var numToFetch = 0;
                if (max)
                {
                    numToFetch = max - items.length;
                }
                var currentField = fields[i];

                if (currentField != lastField)
                {
                    start = 0;
                }
                if (currentField == "itemFields")
                {
                    var columns = ["id", "recid", "status", "title"];
                    if (details)
                        columns.push("description");

                    recs = db.query__fts("item", null, term, columns, numToFetch + 1, start);
                }
                else
                {
                    recs = searchWitJoins(db, currentField, (currentField == "stamp" ? "name" : "text"), term, numToFetch, start, details);
                }
                if (recs.length > numToFetch)
                {
                    recs = recs.slice(0, numToFetch);
                    more = true;
                }
            }

            if (recs && recs.length > 0)
            {
                vv.each(recs, function (i, v)
                {
                    items.push(createSearchRecordItem(v, details, currentField));
                });

            }
        }
        more = more || (items.length > max) || (done == false);
        return { "items": items.slice(0, max), "more": more, "start": start };

    };
    this.getVersionControlMatches = function (text, max, details, skip, lastField, restart)
    {

        var items = [];
        var doFullHidLookup = true;
        var start = skip || 0;
        var index = 0;
        max = parseInt(max);
        var more = false;
        var self = this;
        var fields = ["id", "tag", "comment", "stamp"];

        var hs = [];
        text = text.trim();
        var hidText = text;

        for (var i = 0; i < fields.length; i++)
        {
            if (fields[i] == lastField)
            {
                index = i;
            }
        }

        for (var i = index; i < fields.length; i++)
        {

            if (!max || items.length <= max)
            {
                var numToFetch = 0;
                if (max)
                {
                    numToFetch = (max + 1) - items.length;
                }

                if (fields[i] != lastField)
                {
                    skip = 0;
                }
                if (fields[i] == "id")
                {
                    if (text.indexOf("'") == 0 || text.indexOf('"') == 0)
                    {
                        doFullHidLookup = false;
                        hidText = hidText.rtrim("'");
                        hidText = hidText.ltrim("'");

                        hidText = hidText.rtrim('"');
                        hidText = hidText.ltrim('"');
                    }

                    try
                    {
                        var s = self.repo.lookup_hid(hidText);

                        if (s)
                            hs.push(s);

                    }
                    catch (ex)
                    {
                        //ignore any errors here since we don't want to return the "not found" error - we have more
                        //lookups to do

                    }

                    if (doFullHidLookup)
                    {
                        numToFetch = (max + 1) - hs.length;
                        var hs2 = this.repo.vc_lookup_hids(hidText);

                        if (hs2)
                        {
                            hs2 = hs2.slice(skip, Math.min(skip + numToFetch, hs2.length));
                            //make sure there are not duplicates from 
                            //look up hid
                            vv.each(hs2, function (i, v)
                            {
                                if (vv.inArray(v, hs) < 0)
                                {
                                    hs.push(v);
                                }
                            });

                        }
                    }

                    vv.each(hs, function (i, v)
                    {
                        var desc = self.repo.get_changeset_description(v);

                        var item = {
                            name: desc.revno + ":" + desc.changeset_id.slice(0, 10),
                            id: v,
                            extra: self.userHash[desc.audits[0].userid] || "",
                            description: (desc.comments[0] ? desc.comments[0].text : " "),
                            category: "changesets",
                            matcharea: "id"
                        }
                        if (details)
                            item.matchtext = desc.revno + ":" + desc.changeset_id;

                        items.push(item);
                    });
                }
                else if (fields[i] == "tag")
                {
                    var db = new zingdb(self.repo, sg.dagnum.VC_TAGS);
                    var tags = db.query("item", ["*"], "tag == " + vv._sqlEscape(hidText));
                    if (tags)
                    {
                        vv.each(tags, function (i, v)
                        {
                            var desc = self.repo.get_changeset_description(v.csid);

                            var item = {
                                name: desc.revno + ":" + desc.changeset_id.slice(0, 10),
                                id: v.csid,
                                extra: self.userHash[desc.audits[0].userid] || "",
                                description: (desc.comments[0] ? desc.comments[0].text : " "),
                                category: "changesets",
                                matcharea: "tag",
                                matchtext: v.tag
                            };

                            items.push(item);

                        });

                    }
                }
                else if (fields[i] == "comment")
                {
                    var term = getSearchText(text);

                    var db = new zingdb(self.repo, sg.dagnum.VC_COMMENTS);
                    var cm = db.query__fts("item", "text", term, ['*'], numToFetch, skip);
                    if (cm.length > max)
                    {
                        cm = cm.slice(0, max);
                        more = true;
                    }
                    vv.each(cm, function (i, v)
                    {
						try{
							var desc = self.repo.get_changeset_description(v.csid);
	
							var item = {
								name: desc.revno + ":" + desc.changeset_id.slice(0, 10),
								id: v.csid,
								extra: self.userHash[desc.audits[0].userid] || "",
								description: v.text,
								category: "changesets",
								matcharea: "comment"
							}
							if (details)
							{
								item.matchtext = v.text;
							}
	
							items.push(item);
						}
						catch(ex){
							// The comment that query__fts() matched was on a changeset that
							// is not present in the repository. Just ignore it.
						}
                    });

                }
                else if (fields[i] == "stamp")
                {
                    //TODO fts stamps
                    db = new zingdb(this.repo, sg.dagnum.VC_STAMPS);

                    var sts = db.query("item", ["*"], vv.where({ 'stamp': hidText }), "last_timestamp #DESC", numToFetch, skip);
                    if (sts.length > max)
                    {
                        sts = sts.slice(0, max);
                        more = true;
                    }
                    if (sts)
                    {
                        vv.each(sts, function (i, v)
                        {
                            var desc = self.repo.get_changeset_description(v.csid);
                            if (!desc.audits[0] || !desc.audits[0].username)
                            {
                                sg.log("username is blank : " + sg.to_json__pretty_print(desc));
                            }
                            var item = {
                                name: desc.revno + ":" + desc.changeset_id.slice(0, 10),
                                id: v.csid,
                                extra: self.userHash[desc.audits[0].userid] || "",
                                description: (desc.comments[0] ? desc.comments[0].text : ""),
                                category: "changesets",
                                matcharea: "stamp",
                                matchtext: v.stamp
                            };

                            items.push(item);
                        });
                    }

                }
                if (fields[i] == fields[fields.length - 1])
                    done = true;
            }
        }

        more = more || items.length > max || (done == false);

        return { "items": items.slice(0, max), "more": more };

    }

    this.getBuildMatches = function (text, max, details, skip, restart)
    {
        zs = new zingdb(this.repo, sg.dagnum.BUILDS);
        var stext = getSearchText(text);
        var more = false;
        max = parseInt(max);
        start = skip || 0;
        var items = [];
        var self = this;


        // See if any builds match the criteria
      
        var buildNameRecs = zs.query__fts("buildname", "name", stext, ['*'], max + 1, start);

        var envs = zs.query("environment", ["*"]);
        var envHash = {};
        if (envs)
        {
            vv.each(envs, function (i, n)
            {
                envHash[n.recid] = n.name;
            });
        }

        var statusvals = zs.query("status", ["*"]);
        var statusHash = {};
        if (envs)
        {
            vv.each(statusvals, function (i, n)
            {
                statusHash[n.recid] = n.name;
            });
        }
        if (buildNameRecs.length > max)
        {
            more = true;
            buildNameRecs = buildNameRecs.slice(0, max);
        }

        if (buildNameRecs && buildNameRecs.length)
        {
            vv.each(buildNameRecs, function (index, buildName)
            {
                var buildrefid = buildName.recid;

                // Get all the associated builds
                var buildRecs = zs.query("build", ["*"],  vv.where({'buildname_ref': buildrefid}), "updated #DESC");

                vv.each(buildRecs, function (i, v)
                {                    
                    items.push({
                        name: buildName.name,
                        id: v.recid,
                        extra: v.started || "",
                        description: envHash[v.environment_ref] + " - " + statusHash[v.status_ref],
                        category: "builds",
                        matcharea: "name",
                        matchtext: buildName.name
                    });

                });
            });
        }

        //  more =  more || items.length > max;

        return { "items": items, "more": more };

    }
    this.doWikiFullTextSearch = function (text, max, details, skip, lastField, restart)
    {
        var term = getSearchText(text);
        var more = false;
        max = parseInt(max);
        start = skip || 0;
        var items = [];
        var self = this;

        var db = new zingdb(this.repo, sg.dagnum.WIKI);

        var columns = ["recid", "title", "text"];

        recs = db.query__fts("page", null, term, columns, max + 1, start);

        if (recs.length > max)
        {
            more = true;
            recs = recs.slice(0, max);
        }
        vv.each(recs, function (i, v)
        {
            items.push({
                name: v.title,
                id: v.title,
                extra: "",
                description: v.text || v.title,
                category: "wiki",
                matcharea: "wiki",
                matchtext: v.text || ""
            });

        });
        return { "items": items, "more": more };
    }
}


function makeSafeSearchString(text)
{
    //just a basic check so quick search doesn't error as often if an unterminated 
    //quote or paren is entered
    //obviously not fool proof
    var qsplits = text.split('"') || [];
    if (qsplits.length)
    {      
        if (((qsplits.length - 1) % 2) != 0)
        {
            text = text.replace(/"/g, '');
        }
    }
    var opsplits = text.split("(") || [];
    var cpsplits = text.split(")") || [];
    if (opsplits.length != cpsplits.length)
    {
        text = text.replace(/\(/g, '');
        text = text.replace(/\)/g, '');
    }
    return text;

}

registerRoutes({
    "/repos/<repoName>/searchadvanced.json": {
        "GET": {

            onDispatch: function (request)
            {
                try
                {
                    var text = vv.getQueryParam("text", request.queryString);
                    var lastField = vv.getQueryParam("lf", request.queryString) || "";

                    var restart = vv.getQueryParam("restart", request.queryString) || false;

                    var areas = "wit,vc,builds,wiki";

                    var skip = vv.getQueryParam("skip", request.queryString) || 0;
                    skip = parseInt(skip);

                    var tmpAreas = vv.getQueryParam("areas", request.queryString);
                    if (tmpAreas)
                        areas = tmpAreas;

                    var max = parseInt(vv.getQueryParam('max', request.queryString)) || 20;

                    //var recs = zs.query("item", ["*"], "id == '" + text.toUpperCase() + "'");
                    var results = {};
                    var sr = new searchRequest(request.repo);
                    if (areas && areas.indexOf("wit") >= 0)
                    {
                        var wititems = sr.doItemFullTextSearch(text, max, true, skip, lastField, restart);
                        results["wit"] = wititems;
                    }

                    if (areas && areas.indexOf("vc") >= 0)
                    {
                        var vcitems = sr.getVersionControlMatches(text, max, true, skip, lastField, restart);
                        results["vc"] = vcitems;
                    }
                    if (areas && areas.indexOf("builds") >= 0)
                    {
                        var builds = sr.getBuildMatches(text, max, true, skip, restart);
                        results["builds"] = builds;
                    }
                    if (areas && areas.indexOf("wiki") >= 0)
                    {
                        var wiki = sr.doWikiFullTextSearch(text, max, true, skip, restart);
                        results["wiki"] = wiki;
                    }
                }
                catch (ex)
                {
                    if (ex.toString().indexOf("sqlite") >= 0)
                    {
                        return jsonResponse({ error: "syntax error in search string" });
                    }
                    else
                    {
                        throw (ex);
                    }

                }
                return jsonResponse(results);
            }

        }

    },
    "/repos/<repoName>/searchresults.json": {
        "GET": {

            onDispatch: function (request)
            {
                var text = vv.getQueryParam("text", request.queryString);

                text = makeSafeSearchString(text);
                var max = parseInt(vv.getQueryParam('max', request.queryString) || 15);

                //var recs = zs.query("item", ["*"], "id == '" + text.toUpperCase() + "'");
                var items = [];
                var vcitems = null;
                var builds = null;
                var wiki = null;
                var maybemore = true;

                try
                {
                    var sr = new searchRequest(request.repo);
                    var wititems = sr.doItemFullTextSearch(text, max, false);

                    items = wititems.items;
                    if (max == 0 || items.length < max)
                    {
                        vcitems = sr.getVersionControlMatches(text, max, false);
                        if (vcitems.items.length > 0)
                        {
                            items = items.concat(vcitems.items);
                        }
                    }

                    if (max == 0 || items.length < max)
                    {
                        builds = sr.getBuildMatches(text, max, false);
                        if (builds.items.length > 0)
                            items = items.concat(builds.items);
                    }

                    if (max == 0 || items.length < max)
                    {
                        wiki = sr.doWikiFullTextSearch(text, max, false);
                        if (wiki.items.length > 0)
                            items = items.concat(wiki.items);
                        maybemore = false;
                    }

                }
                catch (ex)
                {
                    if (ex.toString().indexOf("sqlite") >= 0)
                    {
                        return jsonResponse({ error: "syntax error in search string" });
                    }
                    else
                    {
                        throw (ex);
                    }

                }

                return jsonResponse({ "items": items, "more": (maybemore || wititems.more || vcitems.more || builds.more || wiki.more) });
            }
        }
    }
});

