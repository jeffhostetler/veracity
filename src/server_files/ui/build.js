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

var _changesetHover = null;

function sortByNameCaseInsensitive(a, b)
{
	var an = a.name.toLowerCase();
	var bn = b.name.toLowerCase();
	if (an < bn)
		return -1;
	if (an > bn)
		return 1;
	return 0;
}

function addUrls(urlRecs)
{
	var i, div, a;

	div = $("#main_content");
	div.append($('<div class="innerheading">Links</div>'));

	var ctr = $('<div class="innercontainer blank"></div>').
		appendTo(div);

	urlRecs.sort(sortByNameCaseInsensitive);

	var ul = $('<ul class="links"></ul>').
		appendTo(ctr);

	for (i in urlRecs)
	{
		a = $(document.createElement("a"));
		a.attr("href", urlRecs[i].url);
		a.text(urlRecs[i].name);

		$('<li></li>').append(a).appendTo(ul);
	}
}

function addDurationDetails(buildRec, historyRecs, statusRecs, statusRec)
{
	var table, tr, td, i, duration, div;
	var addedAtLeastOneRow = false;

	var header = $('<div class="innerheading">Duration Details</div>');

	var ctr = $('<div class="innercontainer blank"></div>');

	table = $(document.createElement("table")).
		appendTo(ctr).
		addClass("contentTable").
		css('width', '100%');

	var start = false, finish = false;
	var iStart;
	var nowMS = new Date().getTime();
	var sum = 0;
	var lastRec = historyRecs.length - 2;
	var iFinish = historyRecs.length - 1;

	if (statusRec.temporary == true)
		lastRec = historyRecs.length - 1;

	/* The build timer starts on the first temporary status with use_for_eta true.
	 * It ends on the first permanent status with use_for_eta true.
	 * Progress bars are only shown for statuses in between. */
	for (i = 0; i <= lastRec; i++)
	{
		if (start == false || finish == false)
		{
			var sr = getRec(statusRecs, historyRecs[i].status_ref);

			if (start == false)
			{
				if (sr.use_for_eta == true && sr.temporary == true)
				{
					start = true;
					iStart = i;
				}
				else
					continue;
			}
			else if (finish == false)
			{
				if (sr.use_for_eta == true && sr.temporary == false)
				{
					iFinish = i;
					finish = true;
				}
			}
		}

		if (i < iFinish)
			sum += parseInt(historyRecs[i+1].updated) - parseInt(historyRecs[i].updated);
		else if (finish == false)
			sum += nowMS - parseInt(historyRecs[historyRecs.length-1].updated);

		if (finish)
			break;
	}

	for (i = 0; i <= lastRec; i++)
	{
		var durationMS;

		if (i < historyRecs.length - 1)
			durationMS = parseInt(historyRecs[i+1].updated) - parseInt(historyRecs[i].updated);
		else
			durationMS = nowMS - parseInt(historyRecs[historyRecs.length-1].updated);

		duration = getDuration(durationMS);

		tr = $(document.createElement("tr"));
		if (i % 2 == 0)
			tr.attr("class", "contentTableAltRow");

		td = $(document.createElement("td"));
		td.text(getRec(statusRecs, historyRecs[i].status_ref).name);
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(duration);
		tr.append(td);

		td = $(document.createElement("td"));

		if (i >= iStart && i <= iFinish)
		{
			if (finish == false || i < iFinish)
			{
				var buildProgress = new progressBar("build");
				td.append(buildProgress.render());
				var percent = Math.round(durationMS / sum * 100);
				buildProgress.setPercentage(percent);
			}
		}

		tr.append(td);

		table.append(tr);
		addedAtLeastOneRow = true;
	}

	if (addedAtLeastOneRow == true)
	{
		div = $("#main_content");
		div.append(header).append(ctr);
	}
}

function addChanges(buildRec, changes, discarded)
{
	var table, tr, td, i, a, div, cs;

	var header = $('<div class="innerheading"></div>');
	var txt = (discarded?"Discarded":"New");
	txt += " Changes Since the Last " + buildRec.series_name + " Build";
	if (buildRec.branchname != null)
		txt += " of " + buildRec.branchname;
	header.text(txt);

	var ctr = $('<div class="innercontainer blank"></div>');

	table = $(document.createElement("table")).appendTo(ctr);
	table.attr("class", "contentTable").css('width', '100%');

	for (i = 0; i < changes.length; i++)
	{
		cs = changes[i];

		tr = $(document.createElement("tr"));
		if (i % 2 == 1)
			tr.attr("class", "contentTableAltRow");

		td = $(document.createElement("td"));
		td.attr("valign", "top");
		a = $(document.createElement("a"));
		a.attr("href", "changeset.html?recid=" + cs.changeset_id);
		a.text(cs.revno + ":" + cs.changeset_id.substr(0, 10));
		_changesetHover.addHover(a, cs.changeset_id);
		td.append(a);
		tr.append(td);

		var hist = firstHistory(cs.comments[0]);
		td = $(document.createElement("td"));
		td.attr("valign", "top");
		td.text(hist.audits[0].username);
		tr.append(td);
		
		td = $(document.createElement("td"));
		var vc = new vvComment(cs.comments[0].text);
		td.html(vc.details);
		tr.append(td);

		table.append(tr);
	}

	div = $("#main_content");
	div.append(header).append(ctr);
}

function addWorkItems(buildRec, bugs)
{
	var table, tr, td, i, a, div, bug;

	table = $(document.createElement("table"));
	table.attr("class", "contentTable");

	tr = $(document.createElement("tr"));
	td = $(document.createElement("th"));
	td.attr("colspan","4");
	td.text("Work Items Addressed in this Build");
	tr.append(td);
	table.append(tr);

	for (i = 0; i < bugs.length; i++)
	{
		bug = bugs[i];

		tr = $(document.createElement("tr"));
		if (i % 2 == 1)
			tr.attr("class", "contentTableAltRow");

		td = $(document.createElement("td"));
		a = $(document.createElement("a"));
		a.attr("href", "workitem.html?recid=" + bug.recid);
		a.text(bug.id);
		//add hover
		_menuWorkItemHover.addHover(a, bug.recid);
		td.append(a);
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(bug.assignee);
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(bug.title);
		tr.append(td);

		td = $(document.createElement("td"));
		td.text(bug.status);
		tr.append(td);

		// TODO: bounce/verify buttons
		// td = $(document.createElement("td"));
		// tr.append(td);

		table.append(tr);
	}

	div = $("#main_content");
	div.append(table);
	div.append($(document.createElement("br")));
}

function getBuildDetail(buildRecId)
{
	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/detail.json",
		dataType : "json",
		data : { buildRecId: buildRecId },

		success : function(data, status, xhr)
		{
			var div, currentStatusRec, a;

			//vCore.consoleLog(JSON.stringify(data, null, 4));

			currentStatusRec = getRec(data.statuses, data.build.status_ref);

			var statusCell = $("#buildStatus");

			if (currentStatusRec.icon != undefined)
			{
				var img = $(document.createElement("img"));
				img.attr("src", sgStaticLinkRoot + "/img/builds/" + currentStatusRec.icon + "64.png");
				statusCell.append(img);
				statusCell.append($(document.createElement("br")));
			}
			statusCell.append(currentStatusRec.name);

			if (data.build.branchname)
				$('#branchName').show();
			else
				$('#branchName').hide();

			$("#branchName").text(data.build.branchname);
			$("#buildSeries").text(data.build.series_name);
			$("#buildEnv").text(data.build.env_name);
			$("#buildName").text(data.build.name);

			var arrowLink = $("#prevBuildArrow");
			if (data.build.prevBuild !== undefined)
			{
				arrowLink.attr("href", "build.html?recid=" + data.build.prevBuild.recid);
				arrowLink.attr("title", "Previous Build: " + data.build.prevBuild.name);
			}
			else
				arrowLink.hide();

			var listLink = $("#backtoListLink");
			var href = "builds.html#s=" + data.build.series_ref + "&b=";
			if (data.build.branchname != null)
				href += encodeURIComponent(data.build.branchname);
			listLink.attr("href", href);

			arrowLink = $("#nextBuildArrow");
			if (data.build.nextBuild !== undefined)
			{
				arrowLink.attr("href", "build.html?recid=" + data.build.nextBuild.recid);
				arrowLink.attr("title", "Next Build: " + data.build.nextBuild.name);
			}
			else
				arrowLink.hide();

			var container = $("#buildSidebar");
			var space = "7px";

			div = $(document.createElement("h4"));
			div.text(getStartedFinishedLabel(data.build, currentStatusRec));
			container.append(div);

			div = $(document.createElement("div"));
			div.text(getStartedFinishedValue(data.build, currentStatusRec));
			container.append(div);

			var label = getEtaDurationLabel(data.build, currentStatusRec);
			if (label != null)
			{
				div = $(document.createElement("h4"));
				div.text(label);
				container.append(div);

				div = $(document.createElement("div"));
				div.text(getEtaDurationValue(data.build, currentStatusRec));
				container.append(div);
			}

			div = $(document.createElement("h4"));
			div.text("Built From Changeset:");
			container.append(div);

			div = $(document.createElement("div"));
			a = $(document.createElement("a"));
			a.attr("href", "changeset.html?recid=" + data.build.cs.hid);
			a.text(data.build.cs.revno + ":" + data.build.cs.hid.substr(0, 10));
			_changesetHover.addHover(a, data.build.cs.hid);
			div.append(a);
			container.append(div);

			if (data.build.others != null && data.build.others.length > 0)
			{
				div = $(document.createElement("h4"));
				div.text("All Environments:");
				container.append(div);

				var table = $(document.createElement("table"));
				var tr, td, img, statusRec;
				for (var i = 0; i < data.build.others.length; i++)
				{
					var b = data.build.others[i];
					statusRec = getRec(data.statuses, b.status_ref);

					tr = $(document.createElement("tr"));

					td = $(document.createElement("td"));
					if (statusRec.icon != null)
					{
						a = $(document.createElement("a"));
						a.attr("href", "build.html?recid=" + b.recid);
						a.attr("title", statusRec.name);
						img = $(document.createElement("img"));
						img.attr("src", sgStaticLinkRoot + "/img/builds/" + statusRec.icon + "16.png");
						a.append(img);
						td.append(a);
					}
					tr.append(td);

					td = $(document.createElement("td"));
					a = $(document.createElement("a"));
					a.attr("href", "build.html?recid=" + b.recid);
					a.attr("title", statusRec.name);
					a.text(b.name);
					td.append(a);
					tr.append(td);

					table.append(tr);
				}

				container.append(table);
			}


			if (data.changes != null) {
				if(data.changes.csRecs.length>0){
					addChanges(data.build, data.changes.csRecs);
				}
				if(data.changes.csRecs_discarded.length>0){
					addChanges(data.build, data.changes.csRecs_discarded, true);
				}
			}

			if (data.bugs != null && data.bugs.length > 0)
				addWorkItems(data.build, data.bugs);

			if (data.history.length > 0)
				addDurationDetails(data.build, data.history, data.statuses, currentStatusRec);

			if (data.build.urls != null && data.build.urls.length > 0)
				addUrls(data.build.urls);

			$("#buildDetailContent").show();
			$(".jtextfill").textfill({ maxFontPixels: 52 });
		},

		complete : function(data, status, xhr)
		{
			$("#buildDetailLoading").hide();
		}
	});
}


function sgKickoff()
{
    _changesetHover = new hoverControl(fetchChangesetHover);
	getBuildDetail(getQueryStringVal("recid"));
}
