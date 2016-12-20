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


var FILTER_RESULTS_TOKEN = "filterResults";
var LAST_QUERY_URL_TOKEN = "sgLastQueryURL";
var NOBODY = "g3141592653589793238462643383279502884197169399375105820974944592";

function getDefaultFilter()
{
    var f = { sort: "priority #DESC", groupedby: "status", columns: "id,title,reporter,priority,logged_time,remaining_time" };
    f.criteria = { "assignee": "currentuser", "milestone": "currentsprint" };

    return f;

}

function witFilterRequest(request, data, session, pageToken)
{
    this.filter = {};
    this.pageToken = pageToken;
   
    this.maxrows = 0;
    this.skip = 0;
    this.recid = vv.getQueryParam("filter", request.queryString);
    if (!this.recid && data && data.filter)
        this.recid = data.filter;
    this.request = request;
    this.data = data;
 
    var self = this;

    function getHashLength(hash)
    {
        var count = 0;
        for (var i in hash)
            count++;

        return count;
    }
    function buildWhereFromFilter(filtervals, db)
    {
        var query = [];

        for (var c in filtervals.criteria)
        {
            var criteria = filtervals.criteria[c];

            if (criteria && criteria.length > 0)
            {
                var wheres = [];

                if (c == "last_timestamp" || c == "created_date")
                {
                    if (criteria != "any")
                    {
                        var date = new Date();
                        date.setHours(0);
                        date.setMinutes(0);
                        date.setSeconds(0);
                        date.setMilliseconds(0);
                        var interval = parseInt(criteria);
                        date.setDate(date.getDate() - interval);

                        w = [c, ">=", date.getTime().toString()];

                        wheres.push(w);
                    }
                }
                else if (c == "estimated")
                {
                    if (criteria == "yes")
                    {
                        wheres.push(["estimate", ">", "0"]);
                    }
                    else if (criteria == "no")
                    {
                        wheres.push([["estimate", "==", "0"], "||", ["estimate", "isnull"]]);
                    }
                }
                else
                {
                    var vals = criteria.split(",");

                    for (var i = 0; i < vals.length; i++)
                    {
                        if (vals[i] == "currentuser")
                        {
                            vals[i] = self.request.headers.From;
                        }
                        if (vals[i] == "currentsprint")
                        {
                            var options = db.query("config", ["*"], vv.where({ key: "current_sprint" }), null, 1);

                            var cs = "";
                            if (options.length > 0)
                            {
                                vals[i] = options[0].value;
                            }
                        }
                        var w = "";
                                            
                        if (vals[i] == "(none)")
                        {
                            if (c == "assignee" || c == "verifier" || c == "reporter")
                            {
                                wheres.push([c, "==", NOBODY]);
                            }
                           
                           w = [[c, "isnull"], "||", [c, "==", '']]; 
                        }
                        else
                        {
                            if (vals[i])
                            {
                                w = [c, "==", vals[i]];
                            }
                        }
                        if (w)
                            wheres.push(w);
                    }
                }

                var orWhere = [];
                for (var i = 0; i < wheres.length; i++)
                {
                    if (wheres[i].length)
                    {
                        var needwrap = (orWhere.length > 0);
                        if (needwrap)
                        {
                            orWhere = [orWhere, "||", wheres[i]];
                        }
                        else
                        {
                            orWhere = wheres[i];
                        }
                    }
                }
                query.push(orWhere);
            }
        }

        var andWhere = [];
        for (var i = 0; i < query.length; i++)
        {
            if (query[i].length)
            {
                var needwrap = (andWhere.length > 0);
                if (needwrap)
                {
                    andWhere = [andWhere, "&&", query[i]];
                }
                else
                    andWhere = query[i];
                // todo SQL quote
            }
        }

        return andWhere;

    }


    this.getFilter = function (repo, zd, id)
    {
        var recid = "";

        if (this.recid)
            recid = this.recid;

        if (id)
            recid = id;

        if (recid == "default")
        {
            this.filter = getDefaultFilter();
        }
        else if (recid)
        {
            var filter = zd.get_record('filter', recid, ["recid", "name", "user", "groupedby", "sort", "columns",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"] }
            ]);

            if (!filter || !filter.recid)
                return false;

            if (filter.criteriatmp && filter.criteriatmp.length)
            {
                var criteria = {};
                vv.each(filter.criteriatmp, function (i, crit)
                {
                    if (crit.fieldid && crit.value)
                    {
                        criteria[crit.fieldid] = crit.value;
                    }

                });
                filter["criteria"] = criteria;
            }
            delete filter.criteriatmp;
            this.filter = filter;

        }
        //var params = vv.getQueryParams(self.request.queryString);
        var params = {};
      
        if (self.data)
            params = self.data;
        else
            params = vv.getQueryParams(self.request.queryString);
        //params = sg.to_json(self.data);

        delete params.tok;
        if (params["maxrows"])
        {
            this.maxrows = parseInt(params["maxrows"]);
            delete params.maxrows;
        }

        if (params["skip"] != undefined)
        {
            this.skip = parseInt(params["skip"]);
            delete params.skip;
        }

        if (params["groupedby"])
        {
            this.filter.groupedby = params["groupedby"];
            delete params.groupedby;
        }
        else
        {
            if (!this.filter.groupedby)
            {
                this.filter.groupedby = "status";
            }
        }

        if (params["sort"])
        {
            this.filter.sort = params["sort"];

            delete params.sort;
        }
        else
        {
            if (!this.filter.sort)
            {
                this.filter.sort = "priority #DESC";
            }
        }

        if (params["columns"])
        {
            this.filter.columns = params["columns"];
            delete params.columns;
        }
        else
        {
            if (!this.filter.columns)
            {
                this.filter.columns = "id,title,reporter,priority";
            }
        }

        if (!recid)
        {
            this.filter.criteria = params;
        }

        return true;
    }


    this.addCriteria = function (zrec, filtercriteria, ztx)
    {
        for (var c in filtercriteria)
        {
            var criteria = filtercriteria[c];
            if (criteria.length > 0)
            {
                var zcrit = ztx.new_record("filtercriteria");
                zcrit.name = zrec.name;
                zcrit.fieldid = c;
                zcrit.value = criteria;
                zcrit.filter = zrec.recid;
            }
        }

        return zrec;
    }

    this.deleteFilter = function (id, user)
    {
        var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

        var rec = db.get_record('filter', id, ["*"]);

        if (!rec)
            return (notFoundResponse());

        if (rec.user != user)
        {
            return (unauthorizedResponse());
        }

        var ztx = db.begin_tx(null, user);

        var crits = db.query('filtercriteria', ['recid'], vv.where({ 'filter': id }));

        vv.each(crits, function (i, c)
        {
            if (c)
            {
                ztx.delete_record('filtercriteria', c.recid);
            }
        });

        ztx.delete_record('filter', id);

        vv.txcommit(ztx);
        ztx = false;
        db.merge(user);

        return (OK);
    }

    this.updateCriteria = function (zrec, newcriteria, db, ztx)
    {
        if (zrec.criteriatmp)
        {
            vv.each(zrec.criteriatmp, function (i, v)
            {
                if (v && v.recid)
                {
                    var rec = db.get_record('filtercriteria', v.recid);

                    //if the saved criteria field is in the
                    //new criteria update it
                    sg.log(rec.fieldid);
                    if (newcriteria && newcriteria[rec.fieldid])
                    {
                        if (newcriteria[rec.fiedid] != rec.value)
                        {
                            var open = ztx.open_record('filtercriteria', rec.recid);
                            open.value = newcriteria[rec.fieldid];
                        }
                    }
                    //if the saved criteria field isn't in the new criteria
                    //delete it
                    else
                    {
                        ztx.delete_record('filtercriteria', v.recid);
                    }
                    delete newcriteria[rec.fieldid];
                }
            });


        }
        zrec.criteria = [];

        delete zrec.criteriatmp;

        //add the left over criteria
        zrec = self.addCriteria(zrec, newcriteria, ztx);
        //return the record leftover criteria to add later
        return zrec;
    }

    this.updateFilter = function (request, filterdata)
    {
        var self = this;
        var filter = null;

        var db;
        var ztx;

        try
        {
            db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
            ztx = db.begin_tx(null, this.request.vvuser);
            var zrec = ztx.open_record('filter', filterdata.recid);
            if (!zrec)
            {
                ztx.abort();
                ztx = false;

                return (notFoundResponse());
            }
            //remove any values not to be copied
            var myrecid = filterdata.recid;
            delete filterdata.recid;
            delete filterdata.rectype;

            //copy over the rest
            for (var f in filterdata)
            {
                if (f != "criteria" && f != "criteriaIds")
                {
                    zrec[f] = filterdata[f];
                }
            }

            vv.txcommit(ztx);
            ztx = false;

            db.merge(this.request.vvuser);

            ztx = db.begin_tx(null, this.request.vvuser);
            zrec = db.get_record('filter', myrecid, ["recid", "name",
             { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["recid", "fieldid", "value"]}]);


            zrec = self.updateCriteria(zrec, filterdata.criteria, db, ztx);

            vv.txcommit(ztx);
            ztx = false;

            db.merge(this.request.vvuser);
            ztx = false;

        }
        finally
        {
            if (ztx)
                ztx.abort();
        }
        return OK;
    };

    this.getRealSortValue = function (val)
    {
        switch (val)
        {
            case "milestone":
            case "sprint": //depreciated
                {
                    return ["milestonestart", "milestonename"];
                }

            case "assignee":
                {
                    return ["assigneename"];
                }

            case "verifier":
                {
                    return ["verifiername"];
                }

            case "reporter":
                {
                    return ["reportername"];
                }
          
            default:
                {
                    return [val];
                }
        }

    };
    this.getResults = function ()
    {
        var records = null;
        var zs = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
        if (!self.getFilter(this.request.repo, zs))
        {
            return ({ error: notFoundResponse() });
        }
        var stampCritera = null;

        if (self.filter.criteria && self.filter.criteria.stamp)
        {
            stampCritera = self.filter.criteria.stamp;
            delete self.filter.criteria.stamp;
        }
        var keyword = null;
        if (self.filter.criteria && self.filter.criteria.keyword)
        {
            keyword = self.filter.criteria.keyword;
            delete self.filter.criteria.keyword;
        }
        var blocking = null;
        if (self.filter.criteria && self.filter.criteria.blocking)
        {
            blocking = true;
            delete self.filter.criteria.blocking;
        }
        var depending = null;
        if (self.filter.criteria && self.filter.criteria.depending)
        {
            depending = true;
            delete self.filter.criteria.depending;
        }
        var where = buildWhereFromFilter(self.filter, zs);
        if (stampCritera && stampCritera.length)
        {
            self.filter.criteria.stamp = stampCritera;
        }
        if (keyword)
        {
            self.filter.criteria.keyword = keyword;
        }
        if (blocking)
        {
            self.filter.criteria.blocking = "true";
        }
        if (depending)
        {
            self.filter.criteria.depending = "true";
        }
        var sort = self.filter.sort || null;

        var addGroupBy = true;
        var sortArray = [];
        if (sort)
        {
            var sortParts = sort.split(" ");
            if (sortParts[0] == self.filter.groupedby)
            {
                addGroupBy = false;
            }
            if (sortParts[0] == "time_logged" || sortParts[0] == "remaining_time")
            {
                sort = null;
            }
            else
            {
                sortParts[0] = self.getRealSortValue(sortParts[0]);
                vv.each(sortParts[0], function (i, v)
                {
                    var sortObj = { "name": v };

                    if (sortParts[1])
                    {
                        sortObj.dir = (sortParts[1] == "#DESC" ? "desc" : "asc");
                    }
                    sortArray.push(sortObj);
                });

            }
        }


        //sort by grouped first so that the table groups are in the right order
        if (addGroupBy && self.filter.groupedby != "none"
			&& self.filter.groupedby != "remaining_time"
            && self.filter.groupedby != "time_logged")
        {
            var gb = self.getRealSortValue(self.filter.groupedby);

            vv.each(gb, function (i, v)
            {
                sortArray.unshift({ "name": v });
            });

        }

        var qObj = {
            "rectype": "item",
            "fields": ["recid", "id", "assignee", "reporter", "verifier", "title", "description", "status", "estimate", "priority", "milestone", "last_timestamp", "created_date",
                                    { "type": "to_me", "rectype": "work", "ref": "item", "alias": "work", "fields": ["amount", "when"] },
			                        { "type": "to_me", "rectype": "stamp", "ref": "item", "alias": "stamp", "fields": ["name"] },
                                    { "type": "from_me", "ref": "milestone", "alias": "milestonename", "field": "name" },
                                    { "type": "from_me", "ref": "milestone", "alias": "milestonestart", "field": "startdate" },
                                    { "type": "username", "userid": "assignee", "alias": "assigneename" },
                                    { "type": "username", "userid": "reporter", "alias": "reportername" },
                                    { "type": "username", "userid": "verifier", "alias": "verifiername"}],

            "sort": sortArray,
            "limit": self.maxrows.toString(),
            "skip": self.skip.toString()

        };
        var recids = [];
        if (keyword)
        {
            var term = getSearchText(keyword);
            var recs = zs.query__fts("item", null, term, ["recid"]);
            vv.each(recs, function (i, r)
            {
                if (vv.inArray(r.recid, recids) < 0)
                {
                    recids.push(r.recid);
                }
            });
            recs = zs.query__fts("comment", "text", term, ["item", "text"]);

            vv.each(recs, function (i, r)
            {
                if (vv.inArray(r.item, recids) < 0)
                {
                    recids.push(r.item);
                }
            });
            if (!recids.length)
                return [];
        }
        if (blocking || depending)
        {
            var blockerWhere = vv.where_q({ relationship: "blocks" });
            if (recids.length > 0)
            {
                if (blocking && depending)
                    blockWhere = [blockWhere, "&&", [["source", "in", recids], "||", ["target", "in", recids]]];
                else if (blocking)
                    blockWhere = [blockWhere, "&&", ["source", "in", recids]];
                else
                    blockWhere = [blockWhere, "&&", ["target", "in", recids]];
            }

            records = zs.q(
                {
                    "rectype": "relation",
                    "fields": ["*"],
                    "where": blockerWhere
                });

            if (!records.length)
                return [];
            vv.each(records, function (i, r)
            {
                if (blocking)
                {
                    if (vv.inArray(r.source, recids) < 0)
                    {
                        recids.push(r.source);
                    }
                }
                if (depending)
                {
                    if (vv.inArray(r.target, recids) < 0)
                    {
                        recids.push(r.target);
                    }
                }
            });
            // var blocks = db.query('relation', ['*'], vv.where({ 'relationship': 'blocks' }));

        }

        if (stampCritera && stampCritera.length)
        {
            var vals = stampCritera.split(",");

            var stampWhere = vv.where_q({ name: vals });
            if (recids.length > 0)
            {
                stampWhere = [stampWhere, "&&", ["recid", "in", recids]];
            }
            records = zs.q(
                {
                    "rectype": "stamp",
                    "fields": [{ "type": "from_me", "ref": "item", "alias": "itemid", "field": "recid"}],
                    "where": stampWhere
                });


            if (!records.length)
                return [];

            recids = [];
            //check for duplicates until bug XXXX is fixed
            vv.each(records, function (i, r)
            {
                if (vv.inArray(r.itemid, recids) < 0)
                {
                    recids.push(r.itemid);
                }

            });

        }
        if (recids.length)
        {
            if (where.length)
            {
                where = [where, "&&", ["recid", "in", recids]];
            }
            else
            {
                where = ["recid", "in", recids];
            }
        }
        if (where.length)
        {
            qObj.where = where;
        }

        records = zs.q(qObj);



        //if skip is 0 or not defined reset the recids in stored in the session			          
        if (self.skip == 0 || !self.skip)
        {
            session[this.pageToken] = [];
        }
        var resultIds = session[this.pageToken] || [];
        vv.each(records, function (i, v)
        {
            resultIds.push(v.recid);
        });
        session[this.pageToken] = resultIds;

        return records;

    }

}

function filterResponse(request, data)
{
    var uservals = null;
    var sprintvals = null;
    var templatevals = null;

    //store the results in the session
    var session = vvSession.get();
    var skip = 0;
    if (data)
        skip = data.skip;
    else
        skip = vv.getQueryParam("skip", request.queryString);
  
    if (!session || (skip > 0 && !session[FILTER_RESULTS_TOKEN]))
    {
        return notFoundResponse(request, "Your filter session has timed out. You will need to refresh your filter result list.");
    }

    var wfr = new witFilterRequest(request, data, session, FILTER_RESULTS_TOKEN);
    var results = wfr.getResults();
    if (results.error)
        return (results.error);

    var zs = new zingdb(request.repo, sg.dagnum.USERS);
    uservals = zs.query("user", ["*"]);
    var zs = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
    sprintvals = zs.query("sprint", ["*"]);
    templatevals = zs.get_template();
    
    var result = { filter: wfr.filter, results: results, users: uservals, sprints: sprintvals, template: templatevals };
    return jsonResponse(result);

}
registerRoutes({
    "/repos/<repoName>/filteredworkitems.json":
		{
		    "GET": {
		        enableSessions: true,
		        onDispatch: function (request)
		        {
		            return filterResponse(request, null);
		        }
		    },
		    "POST": {
		        enableSessions: true,
		        availableOnReadonlyServer: true,	       
		        onJsonReceived: function (request, data)
		        {
		            return filterResponse(request, data);
		        }
		    }

		},
    "/repos/<repoName>/filtervalues.json":
		{
		    "GET": {
		        onDispatch: function (request)
		        {
		            var zs = new zingdb(request.repo, sg.dagnum.USERS);
		            var uservals = zs.query("user", ["*"]);
		            zs = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

		            var sprintvals = zs.query("sprint", ["*"], null, "startdate");

		            var templatevals = zs.get_template();

		            var options = zs.query("config", ["*"], vv.where({ key: "current_sprint" }), null, 1);

		            var cs = "";
		            if (options.length > 0)
		            {
		                cs = options[0].value;
		            }

		            var response = jsonResponse({
		                users: uservals,
		                sprints: sprintvals,
		                template: templatevals,
		                currentsprint: cs
		            });
		            return response;
		        }
		    }
		},
    "/repos/<repoName>/filteredworkitems/session.json": {

        "POST": {
            enableSessions: true,
            requireUser: true,
            availableOnReadonlyServer: true,	     
            onJsonReceived: function (request, data)
            {              
                if (!data.recid)
                    return (badRequestResponse());

                var session = vvSession.get();
                var ids = session[FILTER_RESULTS_TOKEN] || [];
                ids.push(data.recid);
                session[FILTER_RESULTS_TOKEN] = ids;
                return jsonResponse(OK);
            }
        }
    },
    "/repos/<repoName>/filteredworkitems/sglastqueryurl.json": {

        "GET": {
            enableSessions: true,
            onDispatch: function (request)
            {
                var session = vvSession.get();
             
                if (!session || !session[LAST_QUERY_URL_TOKEN])
                {
                    return notFoundResponse(request, "Your filter session has timed out. You will need to refresh your filter result list.");
                }

                var url = session[LAST_QUERY_URL_TOKEN];

                return jsonResponse(url);
            }
        },
        "POST": {
            enableSessions: true,
            availableOnReadonlyServer: true,	     
            onJsonReceived: function (request, data)
            {              
                var session = vvSession.get();
                session[LAST_QUERY_URL_TOKEN] = data.url;
                return jsonResponse(OK);
            }
        }
    },
    "/repos/<repoName>/filter/<recid>.json":
		{
		    "PUT":
			{
			    requireUser: true,
			    onJsonReceived: function (request, data)
			    {
			        if ((data.rectype) && (data.rectype != 'filter'))
			            return (badRequestResponse());

			        var wfr = new witFilterRequest(request);
			        var ret = wfr.updateFilter(request.recid, data);
			        return (jsonResponse(ret));
			    }

			},
		    "DELETE":
			{
			    requireUser: true,
			    onDispatch: function (request)
			    {
			        var wfr = new witFilterRequest(request);

			        return (wfr.deleteFilter(request.recid, request.vvuser));
			    }

			}
		},

    "/repos/<repoName>/filters.json":
		{
		    "GET": {
		        onDispatch: function (request)
		        {
		            var max = 0;
		            if (vv.queryStringHas(request.queryString, "max"))
		            {
		                max = parseInt(vv.getQueryParam("max", request.queryString));
		            }

		            var zs = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
		            var from = request.vvuser;
		            var filtervals = {};

		            if (vv.queryStringHas(request.queryString, "details"))
		            {
		                filtervals = zs.query("filter",
											  ["name", "user", "groupedby", "sort", "columns", "recid",
											   { "type": "to_me", "rectype": "filtercriteria", "ref": "filter", "alias": "criteriatmp", "fields": ["fieldid", "value"]}],
											  vv.where({ 'user': from }), 'last_timestamp #DESC');

		                vv.each(filtervals, function (i, v)
		                {
		                    if (v.criteriatmp)
		                    {
		                        var criteria = {};
		                        vv.each(v.criteriatmp, function (index, crit)
		                        {
		                            if (crit.fieldid && crit.value)
		                            {
		                                criteria[crit.fieldid] = crit.value;
		                            }
		                        });
		                        v["criteria"] = criteria;
		                    }
		                    delete v.criteriatmp;
		                });

		            }
		            else
		            {
		                var wh = vv.where({ 'user': from });
		                if (max)
		                {
		                    filtervals = zs.query("filter", ["*"], wh, 'last_timestamp #DESC', max);
		                }
		                else
		                {
		                    filtervals = zs.query("filter", ["*"], wh, 'last_timestamp #DESC');
		                }
		            }
		            var response = jsonResponse(filtervals);

		            return response;
		        }
		    },
		    "POST": {
		        requireUser: true,
		        onJsonReceived: function (request, data)
		        {
		            var response = false;
		            // var wfr = new witFilterRequest(request);
		            var zs = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
		            var ztx = zs.begin_tx(null, request.vvuser);
		            var zrec = ztx.new_record("filter");
		            var id = zrec.recid;
		            zrec.name = data.name;
		            zrec.columns = data.columns; //todo get this from page
		            zrec.sort = data.sort; //todo get this from page
		            zrec.groupedby = data.groupedby;

		            for (var c in data.criteria)
		            {
		                var criteria = data.criteria[c];
		                if (criteria.length > 0)
		                {
		                    var zcrit = ztx.new_record("filtercriteria");
		                    zcrit.name = data.name;
		                    zcrit.fieldid = c;
		                    zcrit.value = criteria;
		                    zcrit.filter = id;
		                }
		            }

		            vv.txcommit(ztx);

		            zs.merge(request.vvuser);
		            response = jsonResponse(id);

		            if (!response)
		                response = badRequestResponse()
		            return response;
		        }
		    }

		}
});
