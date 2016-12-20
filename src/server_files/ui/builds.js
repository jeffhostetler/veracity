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

var statusRecs;
var envRecs;
var loadingBuilds = true;
var allRetrieved = false;
var ignoreHashChange = false;

var hashVals = $.deparam.fragment();
var seriesRecId = hashVals.s;
var defaultSeriesRecId;
var branchname = hashVals.b;
var defaultBranchname;

var oldestFetched = null;
var newestFetched = null;
var lastBuildUpdate = null;
var tempBuilds = {};
var newerBuildsETag = "";

function resetBuildSummaryState()
{
	oldestFetched = null;
	newestFetched = null;
	lastBuildUpdate = null;
	tempBuilds = {};
}

function getBuildCellAndStatus(buildRec)
{
	/* Each cell looks like this:
	 * <td>
	 *   <table>
	 *     <tr>
	 *       <td>Status Icon</td>
	 *       <td>Status Text</td>
	 *     </tr>
	 *     <tr>
	 *       <td>Started/Finished:</td>
	 *       <td>datetime</td>
	 *     </tr>
	 *     <tr>
	 *       <td>ETA/Duration:</td>
	 *       <td>datetime</td>
	 *     </tr>
	 *   </table>
	 * </td>
	 */

	var statusRec = getRec(statusRecs, buildRec.status_ref);
	var cell = $(document.createElement("td"));
	cell.attr("id", buildRec.recid);

	cell.addClass("buildSummary");

	if (statusRec.color)
		cell.addClass('buildstatus' + statusRec.color);

	var subTable = $(document.createElement("table"));
	var tr = $(document.createElement("tr"));

	var td = $(document.createElement("td"));
	td.attr("rowspan","3");
	td.attr("valign", "top");
	var a = $(document.createElement("a"));
	a.attr("href", "build.html?recid=" + buildRec.recid);
	if (statusRec.icon != undefined)
	{
		var img = $(document.createElement("img"));
		img.attr("src", sgStaticLinkRoot + "/img/builds/" + statusRec.icon + "16.png");
		a.append(img);
		td.append(a);
	}
	tr.append(td);

	td = $(document.createElement("td"));
	td.attr("class", "buildStatus");
	a = $(document.createElement("a"));
	a.attr("href", "build.html?recid=" + buildRec.recid);
	a.text(statusRec.name);
	td.append(a);
	tr.append(td);

	subTable.append(tr);

	tr = $(document.createElement("tr"));
	td = $(document.createElement("td"));

	var div = $(document.createElement("div"));
	div.attr("class", "smallFieldLabel");
	div.text(getStartedFinishedLabel(buildRec, statusRec));
	td.append(div);
	td.append(getStartedFinishedValue(buildRec, statusRec));
	tr.append(td);
	subTable.append(tr);

	tr = $(document.createElement("tr"));
	td = $(document.createElement("td"));
	div = $(document.createElement("div"));
	div.attr("class", "smallFieldLabel");
	var label = getEtaDurationLabel(buildRec, statusRec);
	if (label !== null)
		div.text(label);
	td.append(div);

	var etaOrDuration = getEtaDurationValue(buildRec, statusRec);
	div = $(document.createElement("div"));
	var tempBuild = null;
	if (statusRec.temporary == 1)
	{
		tempBuild =
		{
			"cell" : cell,
			"etaDiv" : div,
			"buildRec" : buildRec,
			"statusRec" : statusRec
		};
	}
	if (etaOrDuration != null)
		div.text(etaOrDuration);
	tr.append(td);
	subTable.append(tr);
	td.append(div);

	cell.append(subTable);

	if (lastBuildUpdate === null || lastBuildUpdate < buildRec.updated)
		lastBuildUpdate = buildRec.updated;

	var returnObj = {
						"cell" : cell,
						"statusRec" : statusRec,
						"tempBuild" : tempBuild
					};

	return returnObj;
}

function buildHeaderRow(summaryTable)
{
	// Build the header row, with a cell for each environment.
	var tr = $(document.createElement("tr"));
	tr.append($(document.createElement("th")));
	$.each(envRecs, function(i, env)
		   {
			   var th = $(document.createElement("th"));
			   th.text(env.name);
			   th.addClass("col" + i);
			   tr.append(th);
		   });
	summaryTable.append(tr);
}

function createBuildSummaryRow(buildNameRec, buildRecs)
{
	var tr = $(document.createElement("tr"));

	var td = $(document.createElement("th")).
		text(buildNameRec.name).
		addClass("buildName").
		attr('scope', 'row');
	tr.append(td);

	if (newestFetched === null || buildNameRec.added > newestFetched)
		newestFetched = buildNameRec.added;

	if (oldestFetched === null || buildNameRec.added < oldestFetched)
		oldestFetched = buildNameRec.added;

	$.each(envRecs, function(j, envRec)
	{
		var i;
		var buildRec = null;

		for (i = 0; i < buildRecs.length; i++)
		{
			if (buildNameRec.recid == buildRecs[i].buildname_ref && envRec.recid == buildRecs[i].environment_ref)
			{
				buildRec = buildRecs[i];
				break;
			}
		}
		if (buildRec !== null)
		{
			var buildData = getBuildCellAndStatus(buildRec, statusRecs);
			if (getEtaString(buildRec))
				tempBuilds[buildRec.recid] = buildData.tempBuild;

			td = buildData.cell;
			td.addClass("col" + j);
			td.attr("id", buildRec.recid);
			tr.append(td);
			envRec.gotOne = true;
		}
		else
		{
			td = $(document.createElement("td"));
			td.attr("id", buildNameRec.recid + "_" + envRec.recid);
			td.addClass("col" + j);
			tr.append(td);
		}
	});

	return tr;
}

function showHideColumns()
{
	if (envRecs === undefined || envRecs === null)
		return;

	$.each(envRecs, function(i, envRec)
	{
		if (envRec.gotOne === undefined || envRec.gotOne === null || envRec.gotOne === false)
			$(".col" + i).hide();
		else
			$(".col" + i).show();
	});
}

function addBranches(branches)
{
	var branchList = $("#branchList");
	var gotCurrent = false;

	function addBranch(bn)
	{
		var li = $(document.createElement("li"));
		var a = $(document.createElement("a"));
		
		var qs = "s=" + seriesRecId + "&b=";
		if (bn != "(none)")
			qs += encodeURIComponent(bn);

		a.data("branchname", bn);
		a.text(bn);
		a.attr("href", "#" + qs);

		a.click({"li":li, "bn":bn},
				  function(event)
				  {
					  if (vCore.whichClicked(event.which) != 1)
						  return;

					  event.preventDefault(); // Don't follow the href.

					  var li = event.data.li;
					  var bn = event.data.bn;

					  $("#branchList").children(".current").removeClass("current");
					  li.addClass("current");

					  $("#buildSummaryTable").empty();
					  $.each(envRecs, function(i, envRec)	{envRec.gotOne = false; });

					  if (bn == "(none)")
						  branchname = null;
					  else
						  branchname = bn;

					  /* Alter series links so their middle/right clicks link to the newly selected branch. */
					  $("#seriesList a").each(function(i, v)
											  {
												  var a = $(v);
												  a.attr("href", "#s=" + a.data("seriesRec") + "&b=" + encodeURIComponent(bn));
											  });

					  ignoreHashChange = true;
					  $.bbq.pushState(qs);
					  resetBuildSummaryState();
					  getOlderBuilds(true);
				  });

		li.append(a);
		branchList.prepend(li);
		return li;
	}

	for (var i = 0; i < branches.length; i++)
	{
		var bn = branches[i];
		var li = addBranch(bn);

		if (!gotCurrent && bn == branchname)
		{
			li.addClass("current");
			gotCurrent = true;
		}
	}

	if (!gotCurrent && branchname != null)
	{
		li = addBranch(branchname);
		li.addClass("current");
		gotCurrent = true;
	}

	li = addBranch("(none)");
	if (!gotCurrent && branchname == null)
		li.addClass("current");
}

function fetchAllBranches()
{
	// TODO show spinner
	$("#branchList").empty();

	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/all-branches.json",
		dataType : "json",

		success : function(data, status, xhr)
		{
			addBranches(data);
		},

		complete : function(xhr, status)
		{
			// TODO hide spinner
		}
	});
}

function getSummaryTable()
{
	var summaryTable = $("#buildSummaryTable");
	if (summaryTable.length === 0)
	{
		summaryTable = $(document.createElement("table"));
		summaryTable.attr("id", "buildSummaryTable");
		summaryTable.addClass("buildSummary");
		$("#buildSummary").append(summaryTable);
	}
	return summaryTable;
}

function getBuildSummary()
{
	var summaryTable, summaryDiv, summarySpinner;

	summaryDiv = $("#buildSummary");
	summarySpinner = $("#buildSummaryLoading");

	summaryDiv.empty();
	summarySpinner.show();

	loadingBuilds = true;

	if (branchname == null)
		branchname = "master";

	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/summary.json",
		dataType : "json",
		data :
		{
			series: seriesRecId,
			branchname: branchname
		},

		success : function(data, status, xhr)
		{
			//vCore.consoleLog(JSON.stringify(data, null, 4));

			if (data.seriesRecId != null && data.environments != null && data.series != null)
			{
				var seriesList = $("#seriesList");
				defaultSeriesRecId = seriesRecId = data.seriesRecId;

				$.each(data.series, function(i, seriesRec)
				{
					var li = $(document.createElement("li"));
					var a = $(document.createElement("a"));

					a.text(seriesRec.name);
					a.data("seriesRec", seriesRec.recid);

					var qs = "s=" + seriesRec.recid + "&b=" + encodeURIComponent(branchname);
					a.attr("href", "#" + qs);

					a.click({"li":li, "seriesRecId":seriesRec.recid},
						function(event)
						{
							if (vCore.whichClicked(event.which) != 1)
								return;

							event.preventDefault(); // Don't follow the href.

							seriesRecId = event.data.seriesRecId;
							var li = event.data.li;

							$("#seriesList").children(".current").removeClass("current");
							li.addClass("current");

							$("#buildSummaryTable").empty();
							$.each(envRecs, function(i, envRec)	{envRec.gotOne = false; });

							/* Alter branch links so their middle/right clicks link to the newly selected series. */
							$("#branchList a").each(
								function(i, v)
								{
									var a = $(v);
									if (a.attr("href") != null) // Don't alter the button that fetches all branches
									{
										var qs = "#s=" + event.data.seriesRecId + "&b=";
										if (a.data("branchname") !== "(none)")
											qs += encodeURIComponent(a.data("branchname"));
										a.attr("href", qs);
									}
								});

							ignoreHashChange = true;
							$.bbq.pushState(qs);
							resetBuildSummaryState();
							getOlderBuilds(true);
						});

					li.append(a);
					seriesList.append(li);

					if (seriesRec.recid == seriesRecId)
						li.addClass("current");
				});

				defaultBranchname = branchname = data.branchname;
				addBranches(data.branches);

				$("#buildFilters").show();

				// Save lookup tables
				envRecs = data.environments;
				statusRecs = data.statuses;

				summaryTable = getSummaryTable();
				buildHeaderRow(summaryTable);

				$.each(data.buildnames,
					function(i, buildname)
					{
						summaryTable.append(createBuildSummaryRow(buildname, data.builds));
					});

				if (data != null && data.buildnames != null && data.buildnames.length > 0)
					$("#center").hide();
			}

			if (data.buildnames == null || data.buildnames.length < 10)
				allRetrieved = true;

			if (data.buildnames == null || data.buildnames.length == 0)
			{
				if (summaryTable)
					summaryTable.hide();
				$("#noBuilds").show();
			}

			window.setInterval(updateTimes, 1000);
			window.setInterval(getNewerBuilds, 60000);
		},

		error : function(xhr, status, err)
		{
			allRetrieved = true;
		},

		complete : function(xhr, status)
		{
			showHideColumns();

			if (!allRetrieved && !isVerticalScrollRequired())
				getOlderBuilds(true);

			summarySpinner.hide();
			loadingBuilds = false;
		}
	});
}

function fadeReplaceTableCell(oldThing, newThing)
{
	if ($.browser.mozilla)
	{
		/* Firefox has issues with fadeIn of table elements: http://bugs.jquery.com/ticket/10416.
		   Adding .css('display', 'table-cell') sort of works, but had a weird flicker.
		   So instead, firefox just doesn't get the nice fade effect. */
		oldThing.replaceWith(newThing);
	}
	else
	{
		oldThing.fadeOut(400, function() 
							{
								newThing.hide();
								oldThing.replaceWith(newThing);
								newThing.fadeIn(400);
							});
	}
}

function getNewerBuilds()
{
	var tempBuildRecIds = [];
	for (var recid in tempBuilds)
		tempBuildRecIds.push(recid);

	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/builds.json",
		dataType : "json",
		type : "GET",
		beforeSend: function(xhr){xhr.setRequestHeader("If-None-Match", newerBuildsETag);},
		data :
		{
			series: seriesRecId,
			branchname: branchname,
			newerthan: newestFetched,
			lastbuild: lastBuildUpdate
		},

		success : function(data, status, xhr)
		{
			//vCore.consoleLog(data, null, 4));
			
			if (xhr.status === 304)
				return;

			newerBuildsETag = xhr.getResponseHeader("ETag");

			if (data.updatedbuilds !== null)
			{
				for (var i = data.updatedbuilds.length - 1; i >= 0; i--)
				{
					var ub = data.updatedbuilds[i];
					var buildData = getBuildCellAndStatus(ub);

					var cell = $("#" + ub.recid);

					if (cell.length !== 0)
					{
						/* This is a build already in the table for which we expected new data. */
						fadeReplaceTableCell(cell, buildData.cell);
						if (buildData.tempBuild === null)
							delete tempBuilds[ub.recid];
						else
							tempBuilds[ub.recid] = buildData.tempBuild;
					}
					else
					{
						/* This is a new build in a previously blank cell. */
						cell = $("#" + ub.buildname_ref + "_" + ub.environment_ref);
						if (cell.length !== 0)
						{
							fadeReplaceTableCell(cell, buildData.cell);
							try {
								getRec(envRecs, ub.environment_ref).gotOne = true;
							}
							catch(ex) {
								/* new environment. do nothing. */
							}
						}
					}
				}
			}

			if (data.buildnames !== null && data.buildnames.length > 0)
			{
				$("#center").hide();
				$("#noBuilds").hide();
				var st = getSummaryTable().show();
				if ($("#buildSummaryTable > tbody > tr").length === 0)
					buildHeaderRow(st);

				for (i = data.buildnames.length - 1; i >= 0; i--)
				{
					var tr = createBuildSummaryRow(data.buildnames[i], data.builds);
					$("#buildSummaryTable > tbody > tr").eq(0).after(tr);
				}
			}
		},

		complete : function(xhr, status)
		{
			showHideColumns();
		}
	});
}

function getOlderBuilds(fetchMoreUntilScrollNecessary)
{
	var spinner;

	allRetrieved = false;

	if (oldestFetched == null)
	{
		$("#noBuilds").hide();
		spinner = $("#buildSummaryLoading");
	}
	else
		spinner = $("#moreBuildsLoading");

	$("#center").show();
	loadingBuilds = true;
	spinner.show();

	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/builds.json",
		dataType : "json",
		data :
		{
			series: seriesRecId,
			branchname: branchname,
			olderthan: oldestFetched
		},

		success : function(data, status, xhr)
		{
			var summaryTable = getSummaryTable();

			//vCore.consoleLog(JSON.stringify(data, null, 4));

			if (oldestFetched === null)
			{
				if (data.buildnames.length === 0)
				{
					summaryTable.hide();
					$("#noBuilds").show();
				}
				else
				{
					summaryTable.show();
					buildHeaderRow(summaryTable);
				}
			}

			$.each(data.buildnames,
				function(i, buildname)
				{
					summaryTable.append(createBuildSummaryRow(buildname, data.builds));
				});

			if (data.buildnames == null || data.buildnames.length < 10)
				allRetrieved = true;

			if (data != null && data.buildnames != null && data.buildnames.length > 0)
				$("#center").hide();
		},

		error : function(xhr, status, err)
		{
			allRetrieved = true;
		},

		complete : function(xhr, status)
		{
			showHideColumns();

			if (fetchMoreUntilScrollNecessary && !allRetrieved && !isVerticalScrollRequired())
				getOlderBuilds(true);

			spinner.hide();
			loadingBuilds = false;
		}
	});
}

function updateTimes()
{
	for (var recid in tempBuilds)
	{
		var tb = tempBuilds[recid];
		var eta = getEtaDurationValue(tb.buildRec, tb.statusRec);
		if (eta != null)
			tb.etaDiv.text(eta);
	}
}

function sgKickoff()
{
	getBuildSummary();

	$(window).scroll(function()
	{
		// Chrome 5.0.x will call this twice:
		// http://code.google.com/p/chromium/issues/detail?id=43958
		// The loadingHistory flag is here to work around that.

		// scrollPosition = $(window).scrollTop();
		if (!loadingBuilds && isScrollBottom() && !allRetrieved)
		{
			getOlderBuilds(false);
		}
	});

	$(window).resize(function()
	{
		if (!loadingBuilds && !allRetrieved && !isVerticalScrollRequired())
			getOlderBuilds(true);
	});

	$(window).bind("hashchange",
		function(e)
		{
			if (!ignoreHashChange)
			{
				hashVals = $.deparam.fragment();

				seriesRecId = hashVals.s;
				if (seriesRecId === null || seriesRecId === undefined)
					seriesRecId = defaultSeriesRecId;
				$("#seriesList").children(".current").removeClass("current");
				$("#seriesList a[href^='#s="+seriesRecId+"']").parent("li").addClass("current");

				branchname = hashVals.b;
				if (branchname === null || branchname === undefined)
					branchname = defaultBranchname;
				if (branchname == "(none)")
					branchname = null;

				$("#branchList").children(".current").removeClass("current");
				$("#branchList a[href$='b="+encodeURIComponent(branchname)+"']").parent("li").addClass("current");

				/* Alter series links so their middle/right clicks link to the newly selected branch. */
				$("#seriesList a").each(
					function(i, v)
					{
						var a = $(v);
						a.attr("href", "#s=" + a.data("seriesRec") + "&b=" + encodeURIComponent(branchname));
					});

				/* Alter branch links so their middle/right clicks link to the newly selected series. */
				$("#branchList a").each(
					function(i, v)
					{
						var a = $(v);
						if (a.attr("href") != null) // Don't alter the button that fetches all branches
						{
							var qs = "#s=" + seriesRecId + "&b=";
							if (a.data("branchname") !== "(none)")
								qs += encodeURIComponent(a.data("branchname"));
							a.attr("href", qs);
						}
					});
				$.each(envRecs, function(i, envRec)	{envRec.gotOne = false; });
				$("#buildSummaryTable").empty();
				resetBuildSummaryState();
				getOlderBuilds(true);
			}
			ignoreHashChange = false;
		});
}
