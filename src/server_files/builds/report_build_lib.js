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

var output = new Object();

String.prototype.ltrim = function (chars)
{
	chars = chars || "\\s";
	return this.replace(new RegExp("^[" + chars + "]+", "g"), "");
};
String.prototype.rtrim = function(chars) {
	chars = chars || "\\s";
	return this.replace(new RegExp("[" + chars + "]+$", "g"), "");
};
String.prototype.trim = function (chars)
{
	return this.rtrim(chars).ltrim(chars);
};

function _addToWhere(wherestr, field, value, op, comparison)
{
	var needParen = wherestr.length > 0;
	var result = '';

	if (comparison == null)
		comparison = "==";

	if (needParen)
	{
		result += "(";
		result += wherestr;
		result += ") " + op + " (";
	}

	result += field + " " + comparison + " " + this._sqlEscape(value) + "";
	if (needParen)
		result += ")";

	return(result);
}
function _sqlEscape(st)
{
	if (st === null)
		return('#NULL');
	else if (! st)
	return("''");
	else if (typeof(st) != 'string')
	return(st);

	st = st.replace(/\\/, "\\\\");
	st = st.replace(/'/, "\\'");

	return("'" + st + "'");
}
function buildWhere(criteria)
{
	var wherestr = '';

	for ( var field in criteria )
	{
		var value = criteria[field];

		if (value && (typeof(value) == 'object') && (value.length !== undefined))
		{
			var orStr = '';

			for ( var i in value )
			{
				orStr = this._addToWhere(orStr, field, value[i], '||');
			}

			var needParen = (wherestr.length > 0);

			if (needParen)
			{
				wherestr = "(" + wherestr + ") && (";
			}
			wherestr += orStr;
			if (needParen)
				wherestr += ")";
		}
		else
		{
			wherestr = this._addToWhere(wherestr, field, value, "&&");
		}
	}

	return(wherestr);
}

function print_parseable(msg)
{
	if (output.human == undefined)
		output.human = new Array();
	output.human[output.human.length] = msg;
}

// TODO remove this date formatting code and use strftime.js
var actMonths = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
function stretch(t)
{
	if (t < 10)
	{
		t = "0" + t;
	}

	return (t);
}
function shortDateTime(d)
{

	var result = shortDate(d) + " at " + shortTime(d);

	return (result);
}
function shortDate(d)
{
	var result = actMonths[d.getMonth()] + " " + d.getDate();
	return (result);
}
function shortTime(d)
{
	var result = stretch(d.getHours()) + ":" + stretch(d.getMinutes());
	return (result);
}
function getDateTime(nMS)
{
	var d = new Date(parseInt(nMS));
	return shortDateTime(d) + ":" + stretch(d.getSeconds());
}

function getSeriesRec(zdb, seriesName)
{
	var recs = zdb.query("series", ["*"], "(name == '" + seriesName + "') || (nickname == '" + seriesName + "')");
	if (recs.length == 0)
	{
		print_parseable("Series not found: " + seriesName);
		return null;
	}
	return recs[0];
}

function getEnvironmentRec(zdb, environmentName)
{
	var recs = zdb.query("environment", ["*"], "(name == '" + environmentName + "') || (nickname == '" + environmentName + "')");
	if (recs.length == 0)
	{
		print_parseable("Environment not found: " + environmentName);
		return null;
	}
	return recs[0];
}

function getStatusRec(zdb, statusName)
{
	var recs = zdb.query("status", ["*"], "(name == '" + statusName + "') || (nickname == '" + statusName + "')");
	if (recs.length == 0)
	{
		print_parseable("Status not found: " + statusName);
		return null;
	}
	return recs[0];
}

function getBuildRec(ztx, buildRecId)
{
	try
	{
		return ztx.open_record("build", buildRecId);
	}
	catch (x)
	{
		print_parseable("Build not found: " + buildRecId);
		output.error = x;
		ztx.abort();
		return null;
	}
}

function addBuildRecToOutput(repo, zdb, buildRecId)
{
	var buildRec     = zdb.get_record("build", buildRecId, ["*"]);
	var buildNameRec = zdb.get_record("buildname", buildRec.buildname_ref, ["*"]);

	var seriesRec = zdb.get_record("series", buildRec.series_ref, ["name"]);
	var statusRec = zdb.get_record("status", buildRec.status_ref, ["name","use_for_eta"]);
	var envRec    = zdb.get_record("environment", buildRec.environment_ref, ["name"]);

	var i;

	buildRec.name = buildNameRec.name;
	buildRec.csid = buildNameRec.csid;
	delete buildRec.buildname_ref;
	delete buildRec.rectype;

	if (buildNameRec.branchname !== undefined)
		buildRec.branch = buildNameRec.branchname;

	if (buildNameRec.priority !== undefined)
		buildRec.priority = buildNameRec.priority;

	buildRec.series = seriesRec.name;
	delete buildRec.series_ref;

	buildRec.environment = envRec.name;
	delete buildRec.environment_ref;

	buildRec.status = statusRec;
	delete buildRec.status_ref;

	if (buildRec.urls !== undefined)
		buildRec.urls = repo.fetch_json(buildRec.urls);

	if (output.build == null && output.builds == null)
	{
		output.build = buildRec;
	}
	else
	{
		if (output.builds == null)
		{
			output.builds = new Array();
			output.builds[0] = output.build;
			delete output.build;
		}
		output.builds[output.builds.length] = buildRec;
	}
}

function normalizePath(path)
{
	return path.replace("\\","/","g");
}
function getFileNameFromPath(normalizedPath)
{
	var parent = normalizedPath.rtrim("/");
	var index = parent.lastIndexOf("/");
	var file = normalizedPath.slice(index + 1, normalizedPath.length);
	return file;
}

/******************************************************************
 * new_build
 ******************************************************************/
function new_build(repo, buildName, seriesName, environmentName, statusName, changesetArg, branchnameArg, priorityArg, customObj)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);
	var seriesRec = getSeriesRec(zdb, seriesName);
	var envRec = getEnvironmentRec(zdb, environmentName);
	var statusRec = getStatusRec(zdb, statusName);

	var changeset, branchname;
	var r, ztx, recs, buildNameRec, buildRec, buildRecId;

	var now = new Date().getTime();

	try
	{
		changeset = repo.lookup_hid(changesetArg);
	}
	catch (x)
	{
		print_parseable(changesetArg + " is not a valid changeset revision ID or HID prefix.");
		return -1;
	}

	/* If a branch name was specified, it must exist. */
	if ((branchnameArg != undefined) && (branchnameArg != null))
	{
		var bdb = new zingdb(repo, sg.dagnum.VC_BRANCHES);
		recs = bdb.query("branch", ["name"], "name == '" + branchnameArg + "'");
		if (recs == null || recs.length == 0)
		{
			print_parseable(branchnameArg + " is not a valid branch.");
			return -1;
		}
		branchname = recs[0].name;
	}
	else
	{
		branchname = null;
	}

	/* If priority was specified, it must be numeric. */
	var priority = 0; // The default is set here because the zing constraint isn't working.
	if (priorityArg != null)
	{
		priority = parseInt(priorityArg);
		if (isNaN(priority))
		{
			print_parseable(priorityArg + " is not a valid priority.");
			return -1;
		}
	}

	/* Series, environment, and status are all required. */
	if (seriesRec == null)
	{
		print_parseable("Series is required.");
		return -1;
	}
	if (envRec == null)
	{
		print_parseable("Environment is required.");
		return -1;
	}
	if (statusRec == null)
	{
		print_parseable("Status is required.");
		return -1;
	}

	/* Series, CSID, branch name, and priority must match existing build name, when there is one. */
	recs = zdb.query("buildname", ["recid","series_ref","csid","branchname","priority"], "name == '" + buildName + "'");
	if (recs != null && recs.length != 0)
	{
		buildNameRec = recs[0];

		if (buildNameRec.series_ref != seriesRec.recid)
		{
			var existingSeriesRec = zdb.get_record("series", buildNameRec.series_ref, ["name"]);
			print_parseable("Build " + buildName + " already exists with series " + existingSeriesRec.name + ".");
			return -1;
		}

		if (changeset != buildNameRec.csid)
		{
			print_parseable("Build " + buildName + " already exists, built from changeset " + buildNameRec.csid + ".");
			return -1;
		}
		if (branchname != buildNameRec.branchname)
		{
			if (buildNameRec.branchname != null)
				print_parseable("Build " + buildName + " already exists with branch name " + buildNameRec.branchname + ".");
			else
				print_parseable("Build " + buildName + " already exists with no branch name.");
			return -1;
		}
		if (buildNameRec.priority != priority)
		{
			print_parseable("Build " + buildName + " already exists with priority " + buildNameRec.priority + ".");
			return -1;
		}

		/* Disallow adding a build for an environment that already exists */
		var where =
			{
				buildname_ref: buildNameRec.recid,
				environment_ref: envRec.recid
			};
		recs = zdb.query("build", ["recid"], buildWhere(where));
		if (recs.length > 0)
		{
			print_parseable("Build " + buildName + " already exists for environment " + envRec.name + ".");
			return -1;
		}

		print_parseable("New instance of build: " + buildName);
	}
	else
	{
		print_parseable("   Creating new build: " + buildName);
	}

	print_parseable("               Series: " + seriesRec.name + " (" + seriesRec.nickname + ")");
	print_parseable("          Environment: " + envRec.name + " (" + envRec.nickname + ")");
	print_parseable("               Status: " + statusRec.name + " (" + statusRec.nickname + ")");
	print_parseable("            Changeset: " + changeset);
	if (branchname != null)
		print_parseable("               Branch: " + branchname);
	print_parseable("             Priority: " + priority);

	ztx = zdb.begin_tx();
	try
	{
		if (buildNameRec == null)
		{
			buildNameRec = ztx.new_record("buildname");
			buildNameRec.name = buildName;
			buildNameRec.csid = changeset;
			buildNameRec.priority = priority;
			buildNameRec.series_ref = seriesRec.recid;

			if (branchname != null)
				buildNameRec.branchname = branchname;
		}
		else
		{
			buildNameRec = ztx.open_record("buildname", buildNameRec.recid);
		}

		buildRec = ztx.new_record("build");
		buildRecId = buildRec.recid;

		buildRec.buildname_ref = buildNameRec.recid;

		buildRec.environment_ref = envRec.recid;
		buildRec.series_ref = seriesRec.recid;
		buildRec.status_ref = statusRec.recid;
		buildRec.started = now;
		buildRec.updated = now;

		if (statusRec.use_for_eta == true)
		{
			if (statusRec.temporary == true)
			{
				print_parseable("       Build Start: " + getDateTime(now));
				buildRec.eta_start = now;
			}
			else
			{
				// If a build is initially added with a non-temporary status,
				// it should never be used to calculate ETA. We store an
				// eta_finish here, and update_build will never write an eta_start,
				// which will keep the build from being used for ETA estimates.
				print_parseable("      Build Finish: " + getDateTime(now));
				buildRec.eta_finish = now;
			}
		}

		if ((customObj != undefined) && (customObj != null))
		{
			buildRec.custom = customObj;
		}

		r = ztx.commit();
		if (r != null && r.errors == null)
		{
			print_parseable("Success.");

			output.tx_result = r;
			addBuildRecToOutput(repo, zdb, buildRecId);
			output.buildrecid = buildRecId;
		}
	}
	finally
	{
		if (r != null && r.errors != null)
		{
			ztx.abort();
			print_parseable("Failure.");
			output.error = r;
			return -1;
		}
	}

	return 0;
}

/******************************************************************
 * update_build
 ******************************************************************/
function update_build(repo, buildRecId, statusName, priorityArg)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);
	var oldStatusRec;

	var ztx, buildRec, buildNameRec, r;
	var now = new Date().getTime();

	if (statusName == null && priorityArg == null)
	{
		print_parseable("Neither status nor priority was specified.");
		return -1;
	}

	var priority = null;
	if (priorityArg != null)
	{
		priority = parseInt(priorityArg);
		if (isNaN(priority))
		{
			print_parseable(priorityArg + " is not a valid priority.");
			return -1;
		}
	}

	var newStatusRec = null;
	if (statusName != null)
	{
		newStatusRec = getStatusRec(zdb, statusName);
		if (newStatusRec == null)
		{
			print_parseable(statusName + " is not a valid status.");
			return -1;
		}
	}

	ztx = zdb.begin_tx();
	try
	{
		buildRec = getBuildRec(ztx, buildRecId);
		if (buildRec == null)
			return -1;

		oldStatusRec = zdb.get_record("status", buildRec.status_ref, ["name","temporary","use_for_eta"]);
		buildNameRec = ztx.open_record("buildname", buildRec.buildname_ref);

		print_parseable("Updating build: " + buildNameRec.name);
		if (newStatusRec != null)
		{
			print_parseable("   from Status: " + oldStatusRec.name);
			print_parseable("     to Status: " + newStatusRec.name);

			/* If eta_finish is already set, do nothing.
			 * If we're changing to a TEMPORARY status with use_for_eta true for the first time, set the build start time.
			 * If we're changing to a PERMANENT status with use_for_eta true for the first time, set the build finish time. */
			if (newStatusRec.use_for_eta == true && buildRec.eta_finish === undefined)
			{
				if (newStatusRec.temporary == true)
				{
					if (buildRec.eta_start === undefined)
					{
						print_parseable("   Build Start: " + getDateTime(now));
						buildRec.eta_start = now;
					}
				}
				else
				{
					print_parseable("  Build Finish: " + getDateTime(now));
					buildRec.eta_finish = now;
				}
			}

			buildRec.status_ref = newStatusRec.recid;
		}

		if (priority != null)
		{
			print_parseable(" from Priority: " + buildNameRec.priority);
			print_parseable("   to Priority: " + priority);

			buildNameRec.priority = priority;
		}

		buildRec.updated = (now);

		r = ztx.commit();
		if (r != null && r.errors == null)
		{
			output.tx_result = r;

			print_parseable("Success.");
		}
	}
	finally
	{
		if (r != null && r.errors != null)
		{
			ztx.abort();
			print_parseable("Failure.");
			output.error = r;
			return -1;
		}
	}

	addBuildRecToOutput(repo, zdb, buildRecId);

	return 0;
}

/******************************************************************
 * add_urls
 ******************************************************************/
function add_urls(repo, buildRecId, args)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);

	var ztx, buildRec, buildNameRec, i, url, name, urlObj, urlArray, r;

	ztx = zdb.begin_tx();
	try
	{
		buildRec = getBuildRec(ztx, buildRecId);
		if (buildRec == null)
			return -1;

		buildNameRec = zdb.get_record("buildname", buildRec.buildname_ref, ["recid","name"]);
		print_parseable(" Updating build: " + buildNameRec.name);

		urlArray = new Array(args.length/2);
		for (i = 0; i < args.length; i += 2)
		{
			url = args[i];
			name = args[i+1];

			print_parseable("     Adding URL: " + url);
			if (name !== null)
				print_parseable("      With Name: " + name);

			urlObj = { url:url };
			if (name !== null && name !== undefined)
				urlObj.name = name;

			urlArray[i/2] = urlObj;
		}

		if (buildRec.urls === undefined)
		{
			buildRec.urls = urlArray;
		}
		else
		{
			var oldUrlArray = repo.fetch_json(buildRec.urls);
			buildRec.urls = oldUrlArray.concat(urlArray);
		}

		r = ztx.commit();
		if (r != null && r.errors == null)
		{
			output.tx_result = r;

			print_parseable("Success.");
		}
	}
	finally
	{
		if (r != null && r.errors != null)
		{
			ztx.abort();
			print_parseable("Failure.");
			output.error = r;
			return -1;
		}
	}

	addBuildRecToOutput(repo, zdb, buildRecId);

	return 0;
}

function add_urls_from_file(repo, buildRecId, filePath)
{
	var lines, re, i, args, firstSpacePos;
	var fileContents = sg.file.read(filePath);

	print_parseable("Reading urls from: " + filePath);

	re = /\r\n|\r|\n/;
	lines = fileContents.trim().split(re);

	args = new Array(lines.length * 2);
	for (i = 0; i < lines.length; i++)
	{
		lines[i] = lines[i].trim();
		firstSpacePos = lines[i].indexOf(' ');
		if (firstSpacePos == -1)
		{
			args[2*i] = lines[i];
			args[2*i+1] = null;
		}
		else
		{
			args[2*i] = lines[i].substr(0, firstSpacePos).trim();
			args[2*i+1] = lines[i].substr(firstSpacePos).trim();
		}
	}

	return add_urls(repo, buildRecId, args);
}

/******************************************************************
 * delete_builds
 ******************************************************************/
function delete_builds(repo, buildRecIds)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);

	var ztx, buildRec, buildNameRec, r, i, iBuild, allBuildRecIds;
	var buildRecId;
	var buildNameRecs = {};
	var found;

	ztx = zdb.begin_tx();
	try
	{
		for (iBuild = 0; iBuild < buildRecIds.length; iBuild++)
		{
			buildRecId = buildRecIds[iBuild];

			buildRec = getBuildRec(ztx, buildRecId);
			if (buildRec == null)
			{
				// Error is printed and ztx is aborted by getBuildRec().
				print_parseable("Transaction rolled back; nothing deleted.");
				return -1;
			}

			buildNameRec = zdb.get_record("buildname", buildRec.buildname_ref, ["recid","name"]);
			if (buildNameRecs[buildNameRec.recid] === undefined)
				buildNameRecs[buildNameRec.recid] = buildNameRec;

			print_parseable("Deleting instance of build: " + buildNameRec.name);

			ztx.delete_record("build", buildRecId);
		}

		// Delete all buildnames that will no longer have corresponding builds.

		// For each buildname that we've touched, retrieve its builds.
		for each (buildNameRec in buildNameRecs)
		{
			found = false;
			allBuildRecIds = zdb.query("build", ["recid"], "buildname_ref=='" + buildNameRec.recid + "'");

			// Is there a build in allBuildRecIds that we haven't deleted?
			for (i = 0; i < allBuildRecIds.length; i++)
			{
				if (buildRecIds.indexOf(allBuildRecIds[i].recid) == -1)
				{
					// We found a record in allBuildRecIds that was not deleted.
					found = true;
					break;
				}
			}
			if (found == false)
			{
				print_parseable("Last instance deleted. Deleting named build: " + buildNameRec.name);
				ztx.delete_record("buildname", buildNameRec.recid);
			}
		}

		r = ztx.commit();
		if (r != null && r.errors == null)
		{
			output.tx_result = r;

			print_parseable("Success.");
		}
	}
	finally
	{
		if (r != null && r.errors != null)
		{
			ztx.abort();
			print_parseable("Failure.");
			output.error = r;
			return -1;
		}
	}

	return 0;
}

/******************************************************************
 * list_builds
 ******************************************************************/
function list_builds(repo, query, sort, limit)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);
	var recs, i;

	try
	{
		recs = zdb.query("build", ["recid"], query, sort, limit);
	}
	catch (x)
	{
		output.error = x;
		return -1;
	}

	if (recs == null || recs.length == 0)
	{
		print_parseable("No builds found.");
		return 0;
	}

	for (i = 0; i < recs.length; i++)
	{
		addBuildRecToOutput(repo, zdb, recs[i].recid);
	}

	return 0;
}

/******************************************************************
 * get_pending_build
 ******************************************************************/
function get_pending_build(repo, environmentArg, firstOrLastArg, statusArgs)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);
	var envRec = getEnvironmentRec(zdb, environmentArg);

	var addSort;
	var lowerSort = firstOrLastArg.toLowerCase();
	if (lowerSort === "oldest")
		addSort = "#ASC";
	else if (lowerSort === "newest")
		addSort = "#DESC";
	else
	{
		print_parseable(firstOrLastArg + " is invalid. Try \"oldest\" or \"newest\".");
		return -1;
	}


	var statusRecIds = new Array(statusArgs.length);
	for (var i = 0; i < statusArgs.length; i++)
	{
		var statusRec = getStatusRec(zdb, statusArgs[i]);
		statusRecIds[i] = statusRec.recid;
	}

	var where = buildWhere({
							   "environment_ref" : envRec.recid,
							   "status_ref" : statusRecIds
						   });

	try
	{
		var recs = zdb.query("build", ["recid",
									   {
										   "type" : "from_me",
										   "ref" : "buildname_ref",
										   "field" : "csid"
									   },
									   {
										   "type" : "from_me",
										   "ref" : "buildname_ref",
										   "field" : "priority"
									   },
									   {
										   "type" : "from_me",
										   "ref" : "buildname_ref",
										   "field" : "added",
										   "alias" : "name_added"
									   }
									  ],
							 where, "priority #DESC, name_added " + addSort + ", started #DESC", 1);

		if (recs != null && recs.length > 0)
		{
			addBuildRecToOutput(repo, zdb, recs[0].recid);

			// To make parsing output easier, put the stuff a builder cares about by itself at the bottom.
			output.buildrecid = recs[0].recid;
			output.buildcsid = recs[0].csid;
		}
	}
	catch (x)
	{
		output.error = x;
		return -1;
	}


	return 0;
}

/******************************************************************
 * update_prev_builds
 ******************************************************************/
function update_prev_builds(repo, buildRecId, findStatusArg, updateToStatusArg)
{
	var zdb = new zingdb(repo, sg.dagnum.BUILDS);

	var findStatusRec = getStatusRec(zdb, findStatusArg);
	var updateToStatusRec = getStatusRec(zdb, updateToStatusArg);

	var buildRec = zdb.get_record("build", buildRecId, ["environment_ref", "series_ref",
														{
															"type" : "from_me",
															"ref" : "buildname_ref",
															"field" : "branchname"
														},
														{
															"type" : "from_me",
															"ref" : "buildname_ref",
															"field" : "added",
															"alias" : "name_added"
														}
													   ]);
	if (buildRec == null)
	{
		print_parseable("Build not found: " + buildRecId);
		return -1;
	}

	var branchname = buildRec.branchname;
	if (buildRec.branchname === undefined)
		branchname = null;

	var where =
		{
			environment_ref: buildRec.environment_ref,
			series_ref: buildRec.series_ref,
			status_ref: findStatusRec.recid,
			branchname: branchname
		};


	var whereStr = buildWhere(where);
	whereStr = _addToWhere(whereStr, "name_added", buildRec.name_added, "&&", "<");

	var updateRecs = zdb.query("build",
							   ["recid",
								{
									"type" : "from_me",
									"ref" : "buildname_ref",
									"field" : "branchname"
								},
								{
									"type" : "from_me",
									"ref" : "buildname_ref",
									"field" : "added",
									"alias" : "name_added"
								}
							   ],
							   whereStr,
							   "name_added #DESC, started #DESC");

	var ztx = zdb.begin_tx();
	try
	{
		for (var i = 0; i < updateRecs.length; i++)
		{
			var updateRec = ztx.open_record("build", updateRecs[i].recid);
			updateRec.status_ref = updateToStatusRec.recid;
		}

		var r = ztx.commit();
		if (r != null && r.errors == null)
		{
			output.tx_result = r;

			print_parseable("Success.");
		}
	}
	finally
	{
		if (r != null && r.errors != null)
		{
			ztx.abort();
			print_parseable("Failure.");
			output.error = r;
			return -1;
		}
	}

	return 0;
}
