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

registerRoutes({
	"/repos/<repoName>/burndown.html": {
		"GET": {
			onDispatch: function (request) {
				var vvt = new vvUiTemplateResponse(request, "scrum", "Burndown Chart");
					
				return(vvt);
			}
		}
	},
	"/repos/<repoName>/workitem.html": {
		"GET": {
			onDispatch: function (request) {
				var vvt = new vvUiTemplateResponse(request, "scrum", "Work Item");
					
				return(vvt);
			}
		}
	},
	"/repos/<repoName>/workitems.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse(request, "scrum", "Work Items");

				return (vvt);
			}
		}
	},
	"/repos/<repoName>/managefilters.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse(request, "scrum", "Filters");

				return (vvt);
			}
		}
	},	
	"/repos/<repoName>/milestones.html": {
		"GET": {
			onDispatch: function (request) {
				var vvt = new vvUiTemplateResponse(request, "scrum", "Milestones");
					
				return(vvt);
			}
		}
	},
	"/repos/<repoName>/tags.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse( request, "scrum", "Tags" );

				return (vvt);
			}
		}
	},
	"/repos/<repoName>/search.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse( request, "scrum", "Search" );

				return (vvt);
			}
		}
	},

	"/repos/<repoName>/burndown.json": 
	{
		"GET": 
		{
			onDispatch: function(request) 
			{
				var fetchFunc = function(request)
				{
					var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
						
					var q = request.queryString || "";
					var queryParams = {};
							
					var sections = q.split('&');
					for ( var i = 0; i < sections.length; ++i )
					{
						var name = "", value = "";
						var parts = sections[i].split('=');

						name = decodeURIComponent(parts[0]);	

						if (parts.length > 1)
							value = decodeURIComponent(parts[1]);
								
						queryParams[name] = value;
					}
							
					var recid = queryParams["recid"];
							
					if (!recid)
						throw notFoundResponse();
							
					var results = [];
							
					if (queryParams["type"] == "item")
					{
						var item = db.get_record("item", recid, ["recid","assignee", "reporter", "history",
						{"type":"to_me","rectype":"work","ref":"item","alias":"work","fields":["amount", "when"] }]);
								
						if (!item)
							throw notFoundResponse();
									
						var items = [item];
					}
					else
					{
						var sprint = db.get_record("sprint", recid, ["recid"]);
								
						if (!sprint)
							throw notFoundResponse();
						
						var recids = [ recid ];
						
						var prev_length = recids.length;
						var found_recids = [ recid ];
						do
						{
							prev_length = recids.length;
							
							var children = db.query("sprint", ["recid"], vv.where( { "parent": found_recids } ));
							_recids = children.map(function(c) { return c.recid });
							recids = recids.concat(_recids);
							found_recids = _recids;
						} while (recids.length > prev_length)
						
						var items = db.query("item", 
							["recid", "assignee", "reporter", "history",
							{"type":"to_me","rectype":"work","ref":"item","alias":"work","fields":["amount", "when"] }], 
							vv.where( { "milestone": recids } ));
					}

							
					var seen = [];

					for (var i = 0; i < items.length; i++)
					{
						var item = items[i];
								
						var events = [];
						if (item.work)
						{
							vv.each(item.work, function(i,v) {
										
								if (!v.amount || !v.when)
									return;
										
								v.rectype = "work";
								events.push([parseFloat(v.when), v]);
							});
						}
								
						for (var j = 0; j < item.history.length; j++)
						{
							var hist_rec = item.history[j];
							if (hist_rec.audits) 
							{
								var audit = hist_rec.audits[0];
							}
							else
								continue;
									
							var rec = request.repo.fetch_json(hist_rec.hidrec);
									
							events.push([parseFloat(audit.timestamp), rec]);
						}
								
						events.sort(function(a,b) { return (a[0] - b[0]); } );
								
						var logged = null;
						var est = null;
						var status = "";
								
						vv.each(events, function(i,v) {
							var when = v[0];
							var rec = v[1];
									
							if (rec.rectype == "work")
							{
								var amt = parseInt(rec.amount) || 0;
								if (!logged)
									logged = amt;
								else
									logged += amt;
							}
							else
							{
								est = parseInt(rec.estimate);
								status = rec.status;
							}
									
							var entry = {
								recid: item.recid,
								date: when,
								status: status,
								assignee: item.assignee || "",
								reporter: item.reporter || "",
								estimate: (est || 0),
								logged_work: logged
							};
									
							results.push(entry);
						});
								
					}
						
					return results;
				};

				var cc = new textCache(request, [ sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL ]);
				return cc.get_response(fetchFunc, this, request);
			}
		}
	},
	"/repos/<repoName>/milestones/current.json": {
		"GET" : {
			onDispatch: function(request)
			{
				var config = new Configuration(request.repo, request.vvuser);
				var current = config.get("current_sprint");
					
				if (!current)
					return(jsonResponse({}));
					
				request.recid = current.value;
					
				var cc = new textCache(request, [ sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL ]);
				return (cc.get_response
				(
					function(request)
					{
						var wir = new witRequest(request);

						var results = wir.fetchRecords("sprint");

						if (! results)
							throw notFoundResponse();

						if (results.length == 1)
							results = results[0];

						return results;
					}, 
					this, request)
				);
			}
		}
	},

	"/repos/<repoName>/milestone/release/<recid>.json": {
		"PUT": {
			requireUser: true,
			onJsonReceived: function (request, data) {
				// Return 400 Bad Request if we dont' have a sprint to release
				if (!request.recid)
				{
					var resp = {
						statusCode: STATUS_CODE__BAD_REQUEST,
						headers: {
							"Content-Length": 0
						}
					};
					return resp;
				}
					
				var ztx = false;
					
				try
				{
					var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
					// Mark the sprint released
					ztx = db.begin_tx(null, request.vvuser);

					var sprint = ztx.open_record('sprint', request.recid);
						
					if (! sprint)
						return notFoundResponse();

					var today = new Date();
					sprint.releasedate = today.getTime();

					if (data.move == "true")
					{
						var q = vv.where({ milestone: request.recid, status: 'open' });
						var recs = db.query('item', ['recid'], q);

						for ( var i = 0; i < recs.length; ++i )
						{
							var rec = ztx.open_record('item', recs[i].recid);

							if (data.new_sprint)
								rec.milestone = data.new_sprint;
							else
								rec.milestone = null;
						}
							
						if ((data.set_current == "1") && data.new_sprint)
						{
							var config = new Configuration(request.repo, request.vvuser);
							config.set("current_sprint", data.new_sprint, ztx);
						}
							
					}

					vv.txcommit(ztx);
					ztx = false;

					db.merge(request.vvuser);
						
					return OK;
				}
				finally
				{
					if (ztx)
						ztx.abort();
				}
			}
		}
	}
});

vv.lookupMilestone = function (msid, repo)
    {
        if (!this._milestones)
        {
            var udb = new zingdb(repo, sg.dagnum.WORK_ITEMS);

            var sprintrecs = udb.query('sprint', ['*']);

            this._milestones = this.buildLookup(sprintrecs);
        }

        if (this._milestones[msid])
            return (this._milestones[msid].name);

        return (false);
    };

