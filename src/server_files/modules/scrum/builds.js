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

function getSeriesRecId(zingDb, strSeries)
{
	var recid = zingDb.query("series", ["recid"], vv.where( {'name': strSeries} ));
	return recid;
}

function addETA(zingDb, buildRec, statusRecs)
{
	var i, statusRec, oldBuildRecs, sum;
	var avgBuildCount = 10;

	if (buildRec.eta_finish !== undefined || buildRec.eta_start === undefined)
		return;

	for (i = 0; i < statusRecs.length; i++)
	{
		if (statusRecs[i].recid == buildRec.status_ref)
		{
			statusRec = statusRecs[i];
			break;
		}
	}
	if (statusRec == null)
		throw("Status not found.");

	if (statusRec.temporary != 1)
		return;

	oldBuildRecs = zingDb.query("build", ["eta_start","eta_finish"],
								vv.where({'series_ref': buildRec.series_ref, 'environment_ref': buildRec.environment_ref, '!eta_start': 0, '!eta_finish': 0}),
								"eta_finish #DESC", avgBuildCount);
	if (oldBuildRecs == null || oldBuildRecs.length == 0)
		return;

	sum = 0;
	for (i = 0; i < oldBuildRecs.length; i++)
		sum += (oldBuildRecs[i].eta_finish - oldBuildRecs[i].eta_start);
	if (sum !== 0)
		buildRec.avg_duration = sum / oldBuildRecs.length;
}

function getBuildDetail(repo, zdb, recid)
{
	var buildRec, csRec, i;

	buildRec = zdb.get_record("build", recid,
							  ["recid","buildname_ref","series_ref","environment_ref","status_ref","eta_start","eta_finish","updated","urls",
							   {
								   "type" : "from_me",
								   "ref" : "buildname_ref",
								   "field" : "name",
								   "alias" : "name"
							   },
							   {
								   "type" : "from_me",
								   "ref" : "buildname_ref",
								   "field" : "added",
								   "alias" : "name_added"
							   },
							   {
								   "type" : "from_me",
								   "ref" : "buildname_ref",
								   "field" : "csid",
								   "alias" : "csid"
							   },
							   {
								   "type" : "from_me",
								   "ref" : "buildname_ref",
								   "field" : "branchname"
							   },
							   {
								   "type" : "from_me",
								   "ref" : "series_ref",
								   "field" : "name",
								   "alias" : "series_name"
							   },
							   {
								   "type" : "from_me",
								   "ref" : "environment_ref",
								   "field" : "name",
								   "alias" : "env_name"
							   }
							  ]);

	csRec = repo.fetch_dagnode(sg.dagnum.VERSION_CONTROL, buildRec.csid);
	buildRec.cs = csRec;

	if (buildRec.urls != undefined)
		buildRec.urls = repo.fetch_json(buildRec.urls);

	var where = vv.where(
		{
			'environment_ref': buildRec.environment_ref,
			'series_ref': buildRec.series_ref,
			'branchname': buildRec.branchname
		}
	);

	where = vv._addToWhere(where, 'name_added', buildRec.name_added, "&&", "<");

	// Get the previous build
	var builds = zdb.query("build",
						   ["recid",
							{
								"type" : "from_me",
								"ref" : "buildname_ref",
								"field" : "name"
							},
							{
								"type" : "from_me",
								"ref" : "buildname_ref",
								"field" : "csid"
							},
							{
								"type" : "from_me",
								"ref" : "buildname_ref",
								"field" : "added",
								"alias" : "name_added"
							},
							{
								"type" : "from_me",
								"ref" : "buildname_ref",
								"field" : "branchname"
							}
						   ],
						   where,
						   "name_added #DESC, started #ASC", 1);
	if (builds.length == 1)
	{
		buildRec.prevBuild = builds[0];
		delete buildRec.prevBuild.name_added;
		delete buildRec.prevBuild.branchname;
	}

	where = vv.where(
		{
			'environment_ref': buildRec.environment_ref,
			'series_ref': buildRec.series_ref,
			'branchname': buildRec.branchname
		}
	);

	where = vv._addToWhere(where, 'name_added', buildRec.name_added, "&&", ">");


	// Get the next build
	builds = zdb.query("build",
					   ["recid",
						{
							"type" : "from_me",
							"ref" : "buildname_ref",
							"field" : "name"
						},
						{
							"type" : "from_me",
							"ref" : "buildname_ref",
							"field" : "csid"
						},
						{
							"type" : "from_me",
							"ref" : "buildname_ref",
							"field" : "added",
							"alias" : "name_added"
						},
							{
								"type" : "from_me",
								"ref" : "buildname_ref",
								"field" : "branchname"
							}
					   ],
					   where,
					   "name_added #ASC, started #ASC", 1);
	if (builds.length == 1)
	{
		buildRec.nextBuild = builds[0];
		delete buildRec.nextBuild.name_added;
		delete buildRec.nextBuild.branchname;
	}

	/* Get other environments' build summaries */
	buildRec.others = zdb.query("build", ["recid", "status_ref",
										  {
											  "type" : "from_me",
											  "ref" : "environment_ref",
											  "field" : "name"
										  }
										 ],
								vv.where( {'buildname_ref': buildRec.buildname_ref, 'series_ref': buildRec.series_ref} ),
								"name");


	return buildRec;
}

function getChangesSinceLastBuild(repo, zdb, buildRec)
{
	var returnObj = null;

	if (buildRec.prevBuild !== undefined)
	{
		returnObj = {};

		try
		{
			var csids = repo.merge_preview(buildRec.prevBuild.csid, buildRec.cs.hid);
			var csids_discarded = repo.merge_preview(buildRec.cs.hid, buildRec.prevBuild.csid);
			returnObj.csRecs = csids;
			returnObj.csRecs_discarded = csids_discarded;
		}
		catch (x)
		{
			return null;
		}
	}

	return returnObj;
}

var CHANGES_PER_BATCH = 20;
function getBugFixesBatch(zdbWIT, csRecs, startAt)
{
	var where = {};
	var i, j, linkedCsRecs;
	var haveBugs = {};
	var ret = null;
	var lasti = Math.min(csRecs.length, startAt + CHANGES_PER_BATCH);

	where.commit = [];
	for (i = startAt; i < lasti; i++)
	{
		if (csRecs[i].changeset_id != undefined)
			where.commit.push(csRecs[i].changeset_id);
	}

	if (where.commit.length != 0)
	{
		// linkedCsRecs = zdbWIT.query("vc_changeset", ["*","!"], vv.where(where));
		// ret = new Array();
		// for (i = 0; i < linkedCsRecs.length; i++)
		// {
		// 	for (j = 0; j < linkedCsRecs[i].items.length; j++)
		// 	{
		// 		ret.push(zdbWIT.get_record("item", linkedCsRecs[i].items[j], ["recid","id","assignee","title","status"]));
		// 	}
		// }
	}

	return ret;
}

function getBugFixes(repo, csRecs)
{
	var zdbWIT = new zingdb(repo, sg.dagnum.WORK_ITEMS);
	var zdbUsers = new zingdb(repo, sg.dagnum.USERS);

	var ret = new Array();
	var haveBugs = {};
	var i, j;

	for (i = 0; i < csRecs.length; i += CHANGES_PER_BATCH)
	{
		var group = getBugFixesBatch(zdbWIT, csRecs, i);
		if (group != null)
		{
			for (j = 0; j < group.length; j++)
			{
				var rec = group[j];
				if (!haveBugs.hasOwnProperty(rec.recid))
				{
					haveBugs[rec.recid] = null;
					if (rec.assignee !== undefined)
						rec.assignee = zdbUsers.get_record("user", rec.assignee).email;
					ret.push(rec);
				}
			}
		}
	}

	return ret;
}

function getDurationDetails(zdb, buildRecID)
{
	var i, rec, count;
	var historyRecs = zdb.get_record_all_versions("build", buildRecID);
	var returnRecs = [];
	var last_status = null;

	count = 0;
	for (i = 0; i < historyRecs.length; i++)
	{
		rec = historyRecs[i];

		// Disregard changes to the record where the status didn't change
		if (last_status == null || (last_status != rec.status_ref))
		{
			last_status = rec.status_ref;
			returnRecs[count++] = rec;
		}

	}

	return returnRecs;
}

function mergeIfNecessary(zdb)
{
	var leaves = zdb.get_leaves();
	if (leaves.length > 1)
	{
		zdb.merge();
		return true;
	}
	return false;
}

function check_for_uniqueness_conflict(request, zdb, recType, newNick, newName, currentRecId)
{
	var where = vv.where({'name': newName});
	where = vv._addToWhere(where, 'nickname', newNick, '||', '==');

	if (currentRecId != null)
		where = vv._addToWhere(where, 'recid', currentRecId, '&&', '!=');

	var recs = zdb.query(recType, ["recid"], where, null, 1);

	if (recs != null && recs.length > 0)
		throw errorResponse(STATUS_CODE__BAD_REQUEST, request, "There is already a record with that alias or name.");
}

function create_record(request, repo, who, recType, receivedRec, assignmentFunc)
{
	try
	{
		var zdb = new zingdb(repo, sg.dagnum.BUILDS);

		check_for_uniqueness_conflict(request, zdb, recType, receivedRec.nickname, receivedRec.name);

		var ztx = zdb.begin_tx(null, who);
		var createRec = ztx.new_record(recType);
		var recid = createRec.recid;

		assignmentFunc(receivedRec, createRec);

		var result = ztx.commit();

		if (result != null && result.errors == null)
		{
			var response = jsonResponse(zdb.get_record(recType, recid));
		}
	}
	finally
	{
		if (result != null && result.errors != null)
			ztx.abort();

		if (result != null && result.errors != null)
			throw(result);
	}
	return response;
}

function get_record(repo, recType, recid, fields)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);
	var leaves = String(repo.fetch_dag_leaves(sg.dagnum.BUILDS));
	var rec = zdb.get_record(recType, recid, fields, leaves);
	rec.csid = leaves;
	var response = jsonResponse(rec);
	return response;
}

function update_record(request, repo, who, recType, receivedRec, assignmentFunc)
{
	try
	{
		var zdb = new zingdb(repo, sg.dagnum.BUILDS);

		check_for_uniqueness_conflict(request, zdb, recType, receivedRec.nickname, receivedRec.name, receivedRec.recid);

		var ztx = zdb.begin_tx(receivedRec.csid, who);
		var updateRec = ztx.open_record(recType, receivedRec.recid);

		assignmentFunc(receivedRec, updateRec);

		var result = ztx.commit();

		if (result != null && result.errors == null)
		{
			var merged = mergeIfNecessary(zdb);
			var response;

			if (merged)
				response = jsonResponse(zdb.get_record(recType, receivedRec.recid));
			else
				response = jsonResponse(receivedRec);
		}
	}
	finally
	{
		if (result != null && result.errors != null)
			ztx.abort();

		if (result != null && result.errors != null)
			throw(result);
	}
	return response;
}

function delete_record(repo, who, recType, recid, request)
{
	try
	{
		var zdb = new zingdb(repo, sg.dagnum.BUILDS);

		/* Prevent deletion of records that are in use */
		var recs;
		if (recType == "series")
		{
			recs = zdb.query("config", ["recid"], vv.where({'manual_build_series': recid}), null, 1);
			if (recs.length > 0)
				throw errorResponse(STATUS_CODE__BAD_REQUEST, request, "Cannot delete: the series is the Manual Build Series.");
			recs = zdb.query("build", ["recid"], vv.where({'series_ref': recid}), null, 1);
			if (recs.length > 0)
				throw errorResponse(STATUS_CODE__BAD_REQUEST, request, "Cannot delete: the series is referenced by at least one build record.");
			recs = zdb.query("buildname", ["recid"], vv.where({'series_ref': recid}), null, 1);
			if (recs.length > 0)
				throw errorResponse(STATUS_CODE__BAD_REQUEST, request, "Cannot delete: the series is referenced by at least one build record.");
		}
		else if (recType == "environment")
		{
			recs = zdb.query("build", ["recid"], vv.where({'environment_ref': recid}), null, 1);
			if (recs.length > 0)
				throw errorResponse(STATUS_CODE__BAD_REQUEST, request, "Cannot delete: the environment is referenced by at least one build record.");
		}
		else if (recType == "status")
		{
			recs = zdb.query("config", ["recid"], vv.where({'manual_build_status': recid}), null, 1);
			if (recs.length > 0)
				throw errorResponse(STATUS_CODE__BAD_REQUEST, request, "Cannot delete: the status is the Manual Build Status.");
			recs = zdb.query("build", ["recid"], vv.where({'status_ref': recid}), null, 1);
			if (recs.length > 0)
				throw errorResponse(STATUS_CODE__BAD_REQUEST, request, "Cannot delete: the status is referenced by at least one build record.");
		}

		var ztx = zdb.begin_tx(null, who);
		ztx.delete_record(recType, recid);
		var result = ztx.commit();
	}
	finally
	{
		if (result != null && result.errors != null)
			ztx.abort();

		if (result != null && result.errors != null)
			throw(result);
	}
	return OK;
}

function get_branches(repo, bAll)
{
	var branchnameArray = [];
	var branchPile = repo.named_branches();
	var allBranches = branchPile.branches;
	var closedBranches = branchPile.closed;

	for (var branchName in allBranches)
	{
		var includeIt = true;
		if (!bAll)
		{
			includeIt = !(closedBranches.hasOwnProperty(branchName));
		}
		if (includeIt)
			branchnameArray.push(branchName);
	}

	branchnameArray.sort(function(x,y)
						{
							var a = String(x).toUpperCase();
							var b = String(y).toUpperCase();
							if (a > b)
								return -1;
							if (a < b)
								return 1;
							return 0;
						});
	return branchnameArray;
}

function get_buildnames(zdb, seriesRecId, branchname, buildsPerPage, olderThan, newerThan)
{
	if (buildsPerPage == null)
		buildsPerPage = 10;

	var where =
		[
			["branchname", "==", branchname],
			"&&",
			["series_ref", "==", seriesRecId]
		];
	if (olderThan !== undefined && olderThan !== null)
	{
		where = [where, "&&", ["added", "<", olderThan.toString()]];
	}
	if (newerThan !== undefined && newerThan !== null)
	{
		where = [where, "&&", ["added", ">", newerThan.toString()]];
	}

	var buildNameRecs = zdb.q({
								"rectype" : "buildname",
								"fields" : ["*"],
								"where" : where,
								"sort" : [
											{
												"name" : "added",
												"dir" : "desc"
											}],
								"limit" : buildsPerPage
							});
	return buildNameRecs;
}

function get_builds(zdb, buildNameRecs, statusRecs)
{
	// Get all builds associated with the provided buildNames
	var buildCriteria = {};
	buildCriteria.buildname_ref = [];
	vv.each(buildNameRecs, function(i, buildName) {buildCriteria.buildname_ref[i] = buildName.recid;});

	var buildRecs = zdb.query("build", ["*"], vv.where(buildCriteria), "updated #DESC");

	for (var i = 0; i < buildRecs.length; i++)
	{
		// For each build rec we're returning where an ETA will be shown, calculate and return it.
		addETA(zdb, buildRecs[i], statusRecs);
	}

	return buildRecs;
}

function get_branchname_from_qs(qs, branchnameArray)
{
	var branchname = qs.branchname;
	if (branchname === undefined || branchname == "" || branchname == "null")
		branchname = null;

	return branchname;
}

registerRoutes({
    "/repos/<repoName>/builds.html":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var vvt = new vvUiTemplateResponse(request, "scrum", "Build Summary");
		        return (vvt);
		    }
		}
	},
    "/repos/<repoName>/build.html":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var vvt = new vvUiTemplateResponse(request, "scrum", "Build Details");
		        return (vvt);
		    }
		}
	},
    "/repos/<repoName>/build-setup.html":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var vvt = new vvUiTemplateResponse(request, "scrum", "Build Configuration");
		        return (vvt);
		    }
		}
	},
    "/repos/<repoName>/builds/all-branches.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var branches = get_branches(request.repo, true);
		        var response = jsonResponse(branches);
		        return response;
		    }
		}
	},
    "/repos/<repoName>/builds/summary.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var i, response, zdb;
		        var statusRecs = null;
		        var environmentRecs = null;
		        var seriesRecs = null;
		        var seriesRecId = null;
		        var branchnameArray = null;
		        var buildRecs = null;
		        var buildNameRecs = null;
		        var qs = vv.parseQueryString(request.queryString);

		        zdb = new zingdb(request.repo, sg.dagnum.BUILDS);

		        seriesRecs = zdb.query("series", ["*"], null, "name");
		        if (qs.series != null)
		            seriesRecId = qs.series;
		        else
		        {
		            if (seriesRecs != null && seriesRecs.length > 0)
		                seriesRecId = seriesRecs[0].recid;
		        }

		        /* Get open branches */
		        branchnameArray = get_branches(request.repo, false);
		        var branchname = get_branchname_from_qs(qs, branchnameArray);

		        if (seriesRecId != null)
		        {
		            environmentRecs = zdb.query("environment", ["*"], null, "name");
		            statusRecs = zdb.query("status", ["*"]);

		            buildNameRecs = get_buildnames(zdb, seriesRecId, branchname);
		            buildRecs = get_builds(zdb, buildNameRecs, statusRecs);
		        }

		        response =
				{
				    seriesRecId: seriesRecId,
				    environments: environmentRecs,
				    series: seriesRecs,
				    statuses: statusRecs,
				    branches: branchnameArray,
				    buildnames: buildNameRecs,
				    builds: buildRecs
				};

		        if (branchname != null)
		            response.branchname = branchname;

		        response = jsonResponse(response);
		        return response;
		    }
		}
	},
    "/repos/<repoName>/builds/builds.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		    	var cacheFunc = function(request)
		    	{
			        var i, response, zdb;
			        var statusRecs = null;
			        var buildRecs = null;
			        var buildNameRecs = null;
					var updatedBuildRecs = null;
			        var buildCriteria = {};

			        var buildsPerPage = 10;
			        var qs = vv.parseQueryString(request.queryString);
			        var seriesRecId = qs.series;
			        var olderThan = isNaN(qs.olderthan) ? null : parseInt(qs.olderthan);
			        var newerThan = isNaN(qs.newerthan) ? null : parseInt(qs.newerthan);
			        var lastBuildUpdate = isNaN(qs.lastbuild) ? null : parseInt(qs.lastbuild);

			        var branchname = get_branchname_from_qs(qs);

			        zdb = new zingdb(request.repo, sg.dagnum.BUILDS);

			        buildNameRecs = get_buildnames(zdb, seriesRecId, branchname, buildsPerPage, olderThan, newerThan);

			        if (buildNameRecs != null && buildNameRecs.length > 0)
			        {
			            statusRecs = zdb.query("status", ["*"]);
			            buildRecs = get_builds(zdb, buildNameRecs, statusRecs);
			        }

			        if (lastBuildUpdate !== null)
			        {
			        	var updatedBuildRecs = zdb.q({
							"rectype" : "build",
							"fields" :
							[
								"recid", "buildname_ref", "series_ref", "environment_ref", "status_ref",
								"started", "updated", "eta_start", "eta_finish",
								{"type":"from_me","ref":"buildname_ref","field":"branchname","alias":"branchname"}
							],
							"where" :
							[
								[["updated", ">", lastBuildUpdate.toString()], "&&",
								["series_ref", "==", seriesRecId]], "&&",
								["branchname", "==", branchname]
							]});


			        	if (updatedBuildRecs.length > 0 && statusRecs === null)
			            	statusRecs = zdb.query("status", ["*"]);

						for (var i = 0; i < updatedBuildRecs.length; i++)
						{
							// For each build rec we're returning where an ETA will be shown, calculate and return it.
							addETA(zdb, updatedBuildRecs[i], statusRecs);
						}
			        }

			        return {
						    "buildnames": buildNameRecs,
						    "builds": buildRecs,
						    "updatedbuilds": updatedBuildRecs
						};
		    	};

		    	var cc = new textCache(request, [sg.dagnum.BUILDS]);
				return(cc.get_possible_304_response(cacheFunc, this, request));
		    }
		}
	},
    "/repos/<repoName>/builds/detail.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		    	fetchFunc = function(request)
		    	{
			        var zdb;
			        var qs = vv.parseQueryString(request.queryString);

			        var buildRec, historyRecs, statusRecs;
			        var changesObj, fixedBugs;

			        zdb = new zingdb(request.repo, sg.dagnum.BUILDS);

			        buildRec = getBuildDetail(request.repo, zdb, qs.buildRecId);

			        changesObj = getChangesSinceLastBuild(request.repo, zdb, buildRec);

			        if (changesObj != null && changesObj.csRecs != null)
			            fixedBugs = getBugFixes(request.repo, changesObj.csRecs);
			        else
			            fixedBugs = null;

			        historyRecs = getDurationDetails(zdb, buildRec.recid);

			        statusRecs = zdb.query("status", ["*"]);

			        addETA(zdb, buildRec, statusRecs);

			        return {
					    "statuses": statusRecs,
					    "build": buildRec,
					    "changes": changesObj,
					    "bugs": fixedBugs,
					    "history": historyRecs
					};
		    	};

		    	var cc = new textCache(request, [sg.dagnum.BUILDS, sg.dagnum.WORK_ITEMS]);
				return(cc.get_response(fetchFunc, this, request));
		    }
		}
	},
    "/repos/<repoName>/builds/changeset-page.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var response = null;
		        var qs = vv.parseQueryString(request.queryString);

		        var zdb = new zingdb(request.repo, sg.dagnum.BUILDS);

		        var config = zdb.query("config", ["recid"], null, null, 1);

		        var environments = zdb.query("environment", ["recid", "name"], null, "name");

		        var builds = zdb.query("build", ["recid",
													{
													    "type": "from_me",
													    "ref": "buildname_ref",
													    "field": "csid"
													},
													{
													    "type": "from_me",
													    "ref": "buildname_ref",
													    "field": "name"
													},
													{
													    "type": "from_me",
													    "ref": "buildname_ref",
													    "field": "branchname"
													},
													{
													    "type": "from_me",
													    "ref": "status_ref",
													    "field": "name",
													    "alias": "statusname"
													},
													{
													    "type": "from_me",
													    "ref": "status_ref",
													    "field": "icon"
													},
													{
													    "type": "from_me",
													    "ref": "environment_ref",
													    "field": "name",
													    "alias": "envname"
													}
												],
										vv.where({'csid': qs.csid}), "name, envname");

		        response = jsonResponse(
				{
				    config: config,
				    environments: environments,
				    builds: builds
				});
		        return response;
		    }
		}
	},

    "/repos/<repoName>/builds/config.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var response;

		        var zdb = new zingdb(request.repo, sg.dagnum.BUILDS);

		        var config = zdb.query("config", ["*"], null, null, 1);
		        if (config.length == 0)
		            config = null;
		        else
		            config = config[0];
		        var series = zdb.query("series", ["*"], null, "nickname");
		        var environments = zdb.query("environment", ["*"], null, "nickname");
		        var statuses = zdb.query("status", ["*"], null, "nickname");

		        response = jsonResponse(
				{
				    config: config,
				    series: series,
				    environments: environments,
				    statuses: statuses
				});
		        return response;
		    }
		},
	    "PUT":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        var response = jsonResponse(receivedRec);

		        if (receivedRec.series != "null" || receivedRec.status != "null")
		        {
		            try
		            {
		                var zdb = new zingdb(request.repo, sg.dagnum.BUILDS);

		                var ztx = zdb.begin_tx(null, request.vvuser);

		                var recs = zdb.query("config", ["recid"], null, null, 1);
		                if (recs.length == 0)
		                    var updateRec = ztx.new_record("config");
		                else
		                    updateRec = ztx.open_record("config", recs[0].recid);

		                if (receivedRec.series != "null")
		                    updateRec.manual_build_series = receivedRec.series;
		                if (receivedRec.status != "null")
		                    updateRec.manual_build_status = receivedRec.status;

		                var result = ztx.commit();

		                if (result != null && result.errors == null)
		                {
		                    var merged = mergeIfNecessary(zdb);

		                    response = jsonResponse(receivedRec);
		                }
		            }
		            finally
		            {
		                if (result != null && result.errors != null)
		                    ztx.abort();

		                if (result != null && result.errors != null)
		                    throw (result);
		            }
		        }
		        return response;
		    }
		}
	},

    /**
    * Create New Build record(s)
    *
    * Post JSON that looks like this:
    * {
    *   "csid" : hid,
    *   "priority" : int_priority,
    *   "envs" :
    *   [
    *     environment_recid,
    *     ...
    *   ]
    * }
    **/
    "/repos/<repoName>/builds.json":
	{
	    "POST":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        var response;

		        try
		        {
		            var zdb = new zingdb(request.repo, sg.dagnum.BUILDS);

		            /* Get series and status for manually requested build(s) */
		            var recs = zdb.query("config", ["manual_build_series", "manual_build_status"], null, null, 1);
		            if (recs.length != 1)
		                throw ("Manual builds aren't configured.");
		            var seriesRecId = recs[0].manual_build_series;

		            /* Get status for new build(s) */
		            var statusRec = zdb.get_record("status", recs[0].manual_build_status, ["recid", "use_for_eta", "temporary"]);
		            if (statusRec == null)
		                throw ("Unable to find manual build status.");

		            var ztx = zdb.begin_tx(null, request.vvuser);
		            var theDate = new Date();
		            var now = theDate.getTime();

		            var newBuildName = ztx.new_record("buildname");
		            var namerecid = newBuildName.recid;

		            /* Name the build with username and date/time */
		            var udb = new zingdb(request.repo, sg.dagnum.USERS);
		            var userRec = udb.get_record("user", request.vvuser, ["prefix"]);
		            if (userRec == null)
		                throw ("Unable to find user: " + request.vvuser);

		            function pad(n){return n<10 ? '0'+n : n}
		            newBuildName.name = userRec.prefix + "_" +
						theDate.getFullYear() + 
						pad(theDate.getMonth()) + 
						pad(theDate.getDate()) + "-" +
						pad(theDate.getHours()) + 
						pad(theDate.getMinutes()) +
						pad(theDate.getSeconds());

		            newBuildName.series_ref = seriesRecId;
		            newBuildName.csid = receivedRec.csid;
		            newBuildName.added = now;
		            newBuildName.priority = receivedRec.priority;

		            // If the csid is currently on a named branch, add the build on that branch
		            var bdb = new zingdb(request.repo, sg.dagnum.VC_BRANCHES);
		            recs = bdb.query("branch", ["name"], vv.where({'csid': receivedRec.csid}));
		            if (recs.length == 1)
		                newBuildName.branchname = recs[0].name;

		            vv.each(receivedRec.envs, function (i, env)
		            {
		                var newBuild = ztx.new_record("build");
		                newBuild.buildname_ref = newBuildName.recid;
		                newBuild.series_ref = seriesRecId;
		                newBuild.environment_ref = env;
		                newBuild.status_ref = statusRec.recid;
		                newBuild.started = now;

		                if (statusRec.use_for_eta == true)
		                {
		                    if (statusRec.temporary == true)
		                    {
		                        newBuild.eta_start = now;
		                    }
		                    else
		                    {
		                        // If a build is initially added with a non-temporary status,
		                        // it should never be used to calculate ETA. We store an
		                        // eta_finish here, and update_build will never write an eta_start,
		                        // which will keep the build from being used for ETA estimates.
		                        newBuild.eta_finish = now;
		                    }
		                }

		            });

		            var result = ztx.commit();

		            if (result != null && result.errors == null)
		            {
		                /* On success, return new build records */
		                var builds = zdb.query("build", ["recid",
														{
														    "type": "from_me",
														    "ref": "buildname_ref",
														    "field": "recid",
														    "alias": "namerecid"
														},
														{
														    "type": "from_me",
														    "ref": "buildname_ref",
														    "field": "csid"
														},
														{
														    "type": "from_me",
														    "ref": "buildname_ref",
														    "field": "name"
														},
														{
														    "type": "from_me",
														    "ref": "buildname_ref",
														    "field": "branchname"
														},
														{
														    "type": "from_me",
														    "ref": "status_ref",
														    "field": "name",
														    "alias": "statusname"
														},
														{
														    "type": "from_me",
														    "ref": "status_ref",
														    "field": "icon"
														},
														{
														    "type": "from_me",
														    "ref": "environment_ref",
														    "field": "name",
														    "alias": "envname"
														}
													],
											   vv.where({'namerecid': namerecid}), "envname");

		                response = jsonResponse(builds);
		            }
		            else
		                throw (result);
		        }
		        catch (ex)
		        {
		            sg.console(sg.to_json__pretty_print(ex));
		            throw (ex);
		        }
		        finally
		        {
		            if (result != null && result.errors != null)
		                ztx.abort();
		        }
		        return response;
		    }
		}
	},

    /* Series CRUD */
    "/repos/<repoName>/builds/series.json":
	{
	    "POST":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        return create_record(request, request.repo, request.vvuser, "series", receivedRec,
					function (receivedRec, createdRec)
					{
					    createdRec.nickname = receivedRec.nickname;
					    createdRec.name = receivedRec.name;
					});
		    }
		}
	},
    "/repos/<repoName>/builds/series/<recid>.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        return get_record(request.repo, "series", request.recid, ["recid", "nickname", "name"]);
		    }
		},

	    "PUT":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        return update_record(request, request.repo, request.vvuser, "series", receivedRec,
					function (receivedRec, updateRec)
					{
					    updateRec.nickname = receivedRec.nickname;
					    updateRec.name = receivedRec.name;
					});
		    }
		},
	    "DELETE":
		{
		    onDispatch: function (request)
		    {
		        return delete_record(request.repo, request.vvuser, "series", request.recid, request);
		    }
		}
	},

    /* Environment CRUD */
    "/repos/<repoName>/builds/env.json":
	{
	    "POST":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        return create_record(request, request.repo, request.vvuser, "environment", receivedRec,
					function (receivedRec, createdRec)
					{
					    createdRec.nickname = receivedRec.nickname;
					    createdRec.name = receivedRec.name;
					});
		    }
		}
	},
    "/repos/<repoName>/builds/env/<recid>.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        return get_record(request.repo, "environment", request.recid, ["recid", "nickname", "name"]);
		    }
		},

    	    "PUT":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        return update_record(request, request.repo, request.vvuser, "environment", receivedRec,
					function (receivedRec, updateRec)
					{
					    updateRec.nickname = receivedRec.nickname;
					    updateRec.name = receivedRec.name;
					});
		    }
		},
	    "DELETE":
		{
		    onDispatch: function (request)
		    {
		        return delete_record(request.repo, request.vvuser, "environment", request.recid, request);
		    }
		}
	},

    /* Status CRUD */
    "/repos/<repoName>/builds/status.json":
	{
	    "POST":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        return create_record(request, request.repo, request.vvuser, "status", receivedRec,
					function (receivedRec, createdRec)
					{
					    createdRec.nickname = receivedRec.nickname;
					    createdRec.name = receivedRec.name;

					    createdRec.temporary = Boolean(receivedRec.temporary);
					    createdRec.successful = Boolean(receivedRec.successful);
					    createdRec.use_for_eta = Boolean(receivedRec.use_for_eta);
					    createdRec.show_in_activity = Boolean(receivedRec.show_in_activity);

					    if (receivedRec.color != undefined)
					        createdRec.color = receivedRec.color;

					    if (receivedRec.icon != undefined)
					        createdRec.icon = receivedRec.icon;
					});
		    }
		}
	},
    "/repos/<repoName>/builds/status/<recid>.json":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        return get_record(request.repo, "status", request.recid,
					["recid", "nickname", "name", "color", "icon", "temporary", "successful", "use_for_eta", "show_in_activity"]);
		    }
		},

	    "PUT":
		{
		    onJsonReceived: function (request, receivedRec)
		    {
		        return update_record(request, request.repo, request.vvuser, "status", receivedRec,
					function (receivedRec, updateRec)
					{
					    updateRec.nickname = receivedRec.nickname;
					    updateRec.name = receivedRec.name;

					    updateRec.temporary = Boolean(receivedRec.temporary);
					    updateRec.successful = Boolean(receivedRec.successful);
					    updateRec.use_for_eta = Boolean(receivedRec.use_for_eta);
					    updateRec.show_in_activity = Boolean(receivedRec.show_in_activity);

					    if (receivedRec.color == undefined)
					    {
					        if (updateRec.color != undefined)
					            updateRec.color = null;
					    }
					    else
					        updateRec.color = receivedRec.color;

					    if (receivedRec.icon == undefined)
					    {
					        if (updateRec.icon != undefined)
					            updateRec.icon = null;
					    }
					    else
					        updateRec.icon = receivedRec.icon;
					});
		    }
		},
	    "DELETE":
		{
		    onDispatch: function (request)
		    {
		        return delete_record(request.repo, request.vvuser, "status", request.recid, request);
		    }
		}
	}

});
