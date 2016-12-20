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

/*
 * Globals
 */

function burndownController()
{
	var self = this;
	this.treeController = null;
	this.summaryController = null;
    this.finishEstimate = null;
	this.form = null;
	
	this._defaults = {
		rectype: "burndown",
		sprint: null,
		user: "",
		type: "hourly"
	};
	
	this.graphDiv = $("#burndown");
	this.sprint = null;
	this.burndownChart = null;
	this.initial = true;
	this.ajaxQueue = null;
	this.estimatedComplete = "Never";
	this.init = function()
	{
		this.burndownChart = new burndownChart($("#burndown"), {
			widthCalculator: function() { self.widthCalculator() },
			heightCalculator: function() { self.heightCalculator() },
			onError: function() { $("#summary").hide(); }
		});
		
		this.summaryController = new summaryController();
		this._drawSidebar();
		this.bindObservers();
		
        this.graphDiv.css("width", this.widthCalculator());
        this.graphDiv.css("height", this.heightCalculator());
		
		this.fetchChartData();
	};
	
	this.widthCalculator = function() {
		return $("#burndownWrapper").width() - 10;
	}
	
	this.heightCalculator = function() {
		var cheight = windowHeight() - $("#burndownouter").offset().top - 40;
		return cheight;
	}
	/**
	 * Create and render the contents of the left hand sidebar
	 */
	this._drawSidebar = function()
	{
		var self = this;
		var vformOptions = {
			rectype: "burndown",
			afterPopulate: function(form) {
				self.formIsReady(form);
			},
			onChange: function(form) {
				self.formDidChange(form);
			}
		};
		
		var sidebar = $('#rightSidebar');
		
		this.form = new vForm(vformOptions);
		var rec = this._getCookieValues();
		this.form.setRecord(rec);
		
		var header = $("<h1 class='outerheading'>Criteria</h1>").
			appendTo(sidebar);

		var ctr = $("<div class='outercontainer editable'></div>").
			appendTo(sidebar);

		ctr.append(this.form.label("user", "User"));
		var sel = this.form.userSelect("user", {
			allowEmptySelection: true,
			emptySelectionText: "(all users)"
		});
		
		$('<div class="innercontainer"></div>').
			append(sel).
			appendTo(ctr);

		var options = [
			{ name: "Hour Burndown", value: "hourly"},
			{ name: "Issue Burndown", value: "issue"}
		];
		
		ctr.append(this.form.label("type", "Type"));

		sel = this.form.select("type", options );

		$('<div class="innercontainer"></div>').
			append(sel).
			addClass('innercontainer').
			appendTo(ctr);
		
		ctr.append(this.form.label("sprint", "Milestone"));

		var cont = $('<div class="innercontainer" id="treeControl"></div>').
			appendTo(ctr);
		
		this.treeController = new SprintTreeControl(cont, { closeReleased: true });
		
		this.form.addCustomField("sprint", this.treeController);

		$('#summary').
			appendTo(sidebar).
			show();

		this.form.ready();
		
	};
	
	/**
	 * Get the cookie values for this page
	 * 
	 * Fetches the cookies to indicate what user and sprint were selected on the
	 * previous page load.
	 */
	this._getCookieValues = function()
	{
		var cname = vCore.getDefaultCookieName();
		var cookie = vCore.getJSONCookie(cname);
		
		var cookievals = {
			rectype: "burndown",
			sprint: (cookie.burndownSprint || sgCurrentSprint),
			user: cookie.burndownUser
		}
		
		var ret = jQuery.extend(true, {}, this._defaults, cookievals);
		return ret;
	},
	
	this._setCookieValues = function()
	{
		var cname = vCore.getDefaultCookieName();
		var rec = this.form.getFieldValues();
		
		var cobj = {
			burndownSprint: rec.sprint,
			burndownUser: rec.user
		}
		
		vCore.setCookieValuesFromObject(cname, cobj, 30)

	},
	
	this.formIsReady = function()
	{
		this.fetchChartData();
	};

	this.formDidChange = function (form)
	{
	    var newValues = this.form.getFieldValues();
	    this._setCookieValues();

	    if (!newValues.sprint)
	        return;

	    this.sprint = this.treeController.getSprint(newValues.sprint);

	    if (!this.sprint)
	        return;

	    var sprintTree = this.treeController.milestoneTree.getSubTree(this.sprint.recid);
	    var sprints = sprintTree.root.getData();

	    this.summaryController.sprint = this.sprint;
	    this.summaryController.sprints = sprints;
	    this.summaryController.user = newValues.user;

	    this.burndownChart.burndownType = newValues.type;
	    this.burndownChart.user = newValues.user;
	    this.burndownChart.options.startDate = new Date(this.sprint.startdate);
	    this.burndownChart.options.endDate = new Date(this.sprint.enddate);

	    if (this.initial || this.form.getRecordValue("sprint") != newValues.sprint)
	    {
	        this.initial = false;
	        this.fetchChartData();
	    }
	    else
	    {
	        this.burndownChart.prepareChart();
	        if (this.burndownChart.estimatedCompletionDate)
	        {
	            this.estimatedComplete = this.burndownChart.estimatedCompletionDate;
	        }
	        this.summaryController.timeline = this.burndownChart.historyTimeline;
	        this.summaryController.showSummary();

	        this.summaryController.populate(this.estimatedComplete);
	    }

	    this.form.setRecord(newValues);
	};
	
	this.bindObservers = function()
	{
		var self = this;
		// Bind Resize Observers
		$('#maincontent').bind('resize', function() { self.burndownChart.draw(); });
		$('#summary').bind('resize', function() { self.burndownChart.draw(); });
	};
	
	this.fetchChartData = function()
	{
		var self = this;
		var record = this.form.getFieldValues();
		
		if ((!record.sprint) || (record.sprint == '')) {
			self.burndownChart.displayError("Select a sprint to display a burndown");
			return;
		}
		
		var wurl = vCore.getOption("currentRepoUrl") + '/burndown.json?type=sprint&recid=' + record.sprint;
			
		this.burndownChart.setLoading("Loading Chart Data...");
		this.summaryController.setLoading("Loading Summary Data...");
		
		if (this.ajaxQueue)
			return;
			
		this.ajaxQueue = new ajaxQueue({ 
			onSuccess: function() {
				self.ajaxQueue = null;
				self.burndownChart.prepareChart();
				if (self.burndownChart.estimatedCompletionDate)
				{
				    self.estimatedComplete = self.burndownChart.estimatedCompletionDate;
				}
				self.summaryController.timeline = self.burndownChart.historyTimeline;
				self.summaryController.populate(self.estimatedComplete);
			}
		});
		
		this.ajaxQueue.addQueueRequest( {
			url : wurl,
			dataType : 'json',
			success : function(data) 
			{
				// TODO find out why the json data is sometimes null
				if (data == null) { self.burndownChart.displayError("An error has occured.  Please refresh the page"); }
				
				self.burndownChart.setChartData(data);
			},
			error : function(xhr, textStatus, errorThrown) 
			{
				vCore.consoleLog("Error: Fetch Chart Data Failed");
				self.burndownChart.displayError("Unable to display chart.");
			}
		});
		
		this.ajaxQueue.execute();
	};

	this.init();
}

function summaryController()
{
	this.sprint = null;
	this.sprints = [];
	this.user = null;
	this.timeline = null;
	this.progress = null;
	
	this.init = function()
	{
		var self = this;
		this.progress = new smallProgressIndicator("summaryLoading", "Loading Summary...");
		$("#summarylist").hide();
		$("#summary").append(this.progress.draw().addClass('outercontainer'));
	};

	this.populate = function (completeDate)
	{
	    if (!this.sprint)
	        return;

	    $("#summary").show();

	    this.hideLoading();

	    if (this.sprint.startdate)
	        var start = vCore.dateToDayStart(new Date(Math.floor(this.sprint.startdate)));
	    if (this.sprint.enddate)
	        var end = vCore.dateToDayEnd(new Date(Math.floor(this.sprint.enddate)));

	    var today = vCore.dateToDayEnd(new Date());

	    var div = $($("#summarylist")[0]);
	    div.empty();

	    var cont = $("<div>").addClass("sprint").addClass("clearfix");

	    $('#sprinttitle').text(this.sprint.name);

	    var dates = $("<div>").addClass("sum");

	    var date_table = $("<table></table>");
	    var getFormattedDate = function (date)
	    {
	        var curYear = new Date().getFullYear();
	        if (curYear == date.getFullYear())
	        {
	            return date.strftime("%a %b %d");
	        }

	        return date.strftime("%a %b %d, %Y");

	    }
	    if (start)
	        date_table.append(this.tableRow("Start Date", getFormattedDate(start)));
	    if (end)
	        date_table.append(this.tableRow("End Date", getFormattedDate(end)));

	    dates.append(date_table);

	    if (start && end)
	    {
	        date_table = $("<table></table>");

	        date_table.append(this.tableRow("Working Days", vCore.workingDays(start, end)));
	        if (end.getTime() >= today.getTime())
	        {
	            var _d = start.getTime() > today.getTime() ? start : today;
	            date_table.append(this.tableRow("Working Days Left", vCore.workingDays(_d, end)));
	        }

	        dates.append(date_table);
	    }
	    cont.append(dates);

	    var issues = $("<div>").addClass("sum");
	    table = $("<table>");

	    if (this.timeline)
	    {
	        var open = this.timeline.getOpenWICount(today);
	        var closed = this.timeline.getClosedWICount(today);
	        var total = this.timeline.getTotalWICount(today);

	        var _sRecids = [];
	        $.each(this.sprints, function (i, s) { _sRecids.push(s.recid); });
	        var sprintRecids = _sRecids.join("%2C");

	        var totalLink = $("<a>").text(total);
	        var url = vCore.repoRoot() + "/workitems.html?milestone=" + sprintRecids + "&groupedby=status&columns=id%2Ctitle%2Cassignee%2Creporter%2Cpriority%2Cmilestone%2Clogged_time%2Cremaining_time&maxrows=100&skip=0";
	        if (this.user)
	        {
	            url += "&assignee=" + this.user;
	        }
	        totalLink.attr({
	            href: url
	        });
	        table.append(this.tableRow("Total Issues", totalLink));

	        var closedLink = $("<a>").text(closed);
	        var url = vCore.repoRoot() + "/workitems.html?milestone=" + sprintRecids + "&status=fixed%2Cverified%2Cwontfix%2Cinvalid%2Cduplicate&groupedby=status&columns=id%2Ctitle%2Cassignee%2Creporter%2Cpriority%2Cmilestone%2Clogged_time%2Cremaining_time&maxrows=100&skip=0";
	        if (this.user)
	        {
	            url += "&assignee=" + this.user;
	        }
	        closedLink.attr({
	            href: url
	        });
	        table.append(this.tableRow("Closed Issues", closedLink));
	        var openLink = $("<a>").text(open);
	        var url = vCore.repoRoot() + "/workitems.html?milestone=" + sprintRecids + "&status=open&groupedby=status&columns=id%2Ctitle%2Cassignee%2Creporter%2Cpriority%2Cmilestone%2Clogged_time%2Cremaining_time&maxrows=100?skip=0";
	        if (this.user)
	        {
	            url += "&assignee=" + this.user;
	        }
	        openLink.attr({
	            href: url
	        });
	        table.append(this.tableRow("Open Issues", openLink));
	        issues.append(table);
	        cont.append(issues);

	        this.timeline.user = this.user;

	        var start_log = this.timeline.getLoggedHours(start);
	        var start_est = this.timeline.getEstimatedHours(start);
	        if (start_est < 0)
	            start_est = 0;

	        var est = this.timeline.getEstimatedHours(end);
	        var log = this.timeline.getLoggedHours(end);
	        var rem = est - log;

	        var hours = $("<div>").addClass("sum");
	        table = $("<table>");
	        table.append(this.tableRow("Starting Estimate", this.scaleTime(start_est - start_log) + "h"));
	        table.append(this.tableRow("Total Estimated Work", this.scaleTime(est - start_log) + "h"));
	        table.append(this.tableRow("Work Logged", this.scaleTime(log - start_log) + "h"));
	        table.append(this.tableRow("Work Remaining", this.scaleTime(rem) + "h"));

	        if (end && end.getTime() >= today.getTime())
	        {
	            remaining_start_date = today >= start ? today : start

	            var required = rem / vCore.workingDays(remaining_start_date, end);
	            table.append(this.tableRow("Required Work Per Day", this.scaleTime(required) + "h"));
	            date_table.append(this.tableRow("Estimated Completion", completeDate));
	        }


	        hours.append(table);
	        cont.append(hours);
	    }
	    div.append(cont);
	};

	this.tableRow = function (title, value)
	{
	    var tr = $("<tr>");
	    tr.append($("<td>").addClass("key").text(title + ": ").css({"vertical-align": "bottom" }));
	    if (typeof value == "string")
	        tr.append($("<td>").css({ "text-align": "right", "white-space": "nowrap", "vertical-align": "bottom" }).text(value));
	    else
	        tr.append($("<td>").css({ "text-align": "right", "white-space": "nowrap", "vertical-align": "bottom" }).append(value));

	    return tr;
	}
	
	this.showSummary = function()
	{
		$('#summarylist').show();
		$('#imghidesum').attr("src",
				sgStaticLinkRoot + "/img/contract.png");
		$('#summary').trigger('resize');
	};
	
	this.hideSummary = function()
	{
		$('#summarylist').hide();
		$('#imghidesum').attr("src",
				sgStaticLinkRoot + "/img/expand.png");
		$('#summary').trigger('resize');
	};
	
	this.scaleTime = function(v)
	{
		var scaled = Math.round(v / 15.0) / 4.0;

		return (scaled);
	};
	
	this.setLoading = function()
	{
		$("#summarylist").empty().hide();
		this.progress.setText("Loading Summary...");
		this.progress.show();
	};
	
	this.hideLoading = function()
	{
		this.progress.setText("");
		this.progress.hide();
		$('#summarylist').show();
	}
	
	this.init();
}


function sgKickoff() {
	vCore.showSidebar();
	var controller = new burndownController();
}