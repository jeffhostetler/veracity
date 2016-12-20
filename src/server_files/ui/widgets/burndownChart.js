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

var burndownChart = function (container, options)
{
    this.container = $(container);
    this.options = $.extend(true, {
        displayPoints: true,
        displayHovers: true,
        displayLegend: true,
        displayWorkRequired: true,
        displayProjectedCompletion: true,
        displayTrend: true,
        onError: null,
        startDate: null,
        endDate: null,
        width: null,
        height: null,
        widthCalculator: null,
        heightCalculator: null,
        dateFormat: "%b %e,<br/> %Y"
    }, (options || {}));

    this.user = null;
    this.rawData = null;
    this.chartData = null;
    this.ticks = [];
    this.progressIndicator = new smallProgressIndicator("burndownLoading", "");
    this.container.append(this.progressIndicator.draw());
    this.ajaxQueue = null;
    this.historyTimeline = null;
    this.estimatedCompletionDate = null;
    this.previousPoint = null; // Used for plothover
}

extend(burndownChart, {

    setChartData: function (data)
    {
        this.rawData = data;
    },

    setUser: function (userid)
    {
        this.user = userid
    },

    prepareChart: function ()
    {
        var self = this;
        $(this.container).empty();

        if (!this.rawData)
        {
            //this.displayError("Unable to display burndown");
            return;
        }

        this.historyTimeline = new burndownTimeline(this.user);

        // Add data to the timeline
        $.each(this.rawData, function (i, entry)
        {
            self.historyTimeline.addRec(entry);
        });

        var startDate = null;
        if (this.options.startDate)
        {
            if (this.options.startDate == "first_estimate")
            {
                var first = this.historyTimeline.getEarliestFirstEstimate()
                if (first)
                    startDate = new Date(first);
            }
            else
                startDate = new Date(this.options.startDate.getTime());

            if (startDate)
                this.historyTimeline.startDate = new Date(startDate.getTime());
        }
        else
        {
            startDate = this.historyTimeline.startDate;
        }

        var endDate = null;
        if (this.options.endDate)
        {
            endDate = new Date(this.options.endDate.getTime());
            this.historyTimeline.endDate = new Date(endDate.getTime());
        }
        else
        {
            endDate = this.historyTimeline.endDate;
        }

        endDate = vCore.dateToDayEnd(endDate);

        if (startDate)
        {
            // Generate the 'ticks' for the graph
            var workingDays = vCore.workingDays(startDate, endDate) - 1;
            var date = new Date(startDate.getTime());

            this.ticks = [];

            if (this.burndownType != "issue")
            {
                var firstTick = new Date(date.getTime());
                this.ticks.push(firstTick);
            }
            while (date.getTime() < endDate.getTime())
            {
                this.ticks.push(vCore.dateToDayEnd(date));
                var date = vCore.nextWorkingDay(date);
            }
        }

        if (this.ticks.length < 2)
        {
            this.displayError("No burndown events");
            return false;
        }

        var generator = null;

        if (this.burndownType == "issue")
        {
            generator = new WIBurndownGenerator(this.historyTimeline, startDate, endDate);
        }
        else
            generator = new HourBurndownGenerator(this.historyTimeline, startDate, endDate);

        generator.ticks = this.ticks;
        generator.init();
        this.estimatedCompletionDate = generator.finishDateEstimate || null;

        if (!this.options.displayTrend)
            generator.displayTrend = false;
        if (!this.options.displayWorkRequired)
            generator.displayWorkRequired = false;
        if (!this.options.displayProjectedCompletion)
            generator.displayProjectedCompletion = false;

        this.chartData = generator.chartData();
        this.draw();

        return true;
    },

    displayError: function (msg)
    {
        if (this.options.onError)
            this.options.onError();

        $(this.container).empty();
        $("canvas").remove();
        var error = $(document.createElement("p")).addClass("burndownError");
        error.text(msg);
        $(this.container).append(error);
    },

    draw: function ()
    {
        if (!this.chartData)
            return;

        var self = this;

        this.container.empty();

        if (this.options.width)
            width = this.options.width;
        else if (this.options.widthCalculator)
            width = this.options.widthCalculator();
        else
            width = this.container.parent().width();

        if (this.options.height)
            height = this.options.height;
        else if (this.options.heightCalculator)
            height = this.options.heightCalculator();
        else
            height = 100;

        this.container.css("width", width);
        this.container.css("height", height);

        var plotOpts = {
            xaxis: {
                tickSize: 1,
                tickFormatter: function (val, axis)
                {
                    var skip = 1;
                    var ticks = self.ticks.length;
                    var tickWidth = width / ticks;

                    if (tickWidth < 40)
                    {
                        skip = Math.ceil(40 / tickWidth)
                    }

                    if (!self.ticks[val])
                        return "";

                    if (ticks > 2 && (((val - 1) % skip) != 0))
                    {
                        return "";
                    }
                    else
                    {
                        if (val == 0 || self.burndownType == "issue")
                            var d = self.ticks[val];
                        else
                            var d = vCore.nextWorkingDay(self.ticks[val])
                        return vCore.formatDate(vCore.dateToDayStart(d), self.options.dateFormat);
                    }
                }
            },
            yaxis: {
                max: this.chartData.yMax,
                tickDecimals: 0
            },
            lines: {
                show: true
            },
			hooks: {
				draw: [function() { self.hideLoading(); }]
			}
        };
             
        if (this.options.xLabelWidth)
            plotOpts.xaxis.labelWidth = this.options.xLabelWidth;

        if (this.options.displayPoints)
        {
            plotOpts.points = { show: true, fill: false };
        }
        if (this.options.displayHovers)
        {
            plotOpts.grid = { hoverable: true };
        }
        if (!this.options.displayLegend)
        {
            plotOpts.legend = { show: false };
        }

        var plot = $.plot(this.container, this.chartData.series, plotOpts);

        if (this.options.displayHovers)
        {
            // Bind the chart hover event to display a tooltip
            this.container.bind(
				"plothover",
				function (event, pos, item)
				{
				    $("#x").text(pos.x.toFixed(2));
				    $("#y").text(pos.y.toFixed(2));

				    if (item)
				    {
				        if (self.previousPoint != item.datapoint)
				        {
				            self.previousPoint = item.datapoint;
				            $("#chartPopup").remove();
				            var x = item.datapoint[0], y = item.datapoint[1];
				            var prefix = "";
				            if (x == 0 || self.burndownType == "issue")
				            {
				                var date = self.ticks[x];
				                var dateStr = date.strftime("%a %b %d");
				            }
				            else
				            {
				                prefix = "As of ";
				                var date = vCore.nextWorkingDay(self.ticks[x]);
				                var dateStr = vCore.dateToDayStart(date).strftime("%a %b %d");
				            }

				            var label = (item.series.label == undefined) ? "" : item.series.label + ": ";
				            var ttCont = "<b>" + prefix + dateStr + "</b><br/>" + label + y;
				            self.showTooltip(item.pageX, item.pageY, ttCont);
				        }
				    } else
				    {
				        $("#chartPopup").remove();
				        self.previousPoint = null;
				    }
				}
			);
        }
    },

    setLoading: function (text)
    {
        $(this.container).empty();
        $("canvas").remove();

        this.progressIndicator.setText(text);
        $(this.container).append(this.progressIndicator.draw());
    },

    hideLoading: function ()
    {
        this.progressIndicator.setText("");
        this.progressIndicator.hide();
    },

    showTooltip: function (x, y, contents)
    {
        var popup = $('<div id="chartPopup">' + contents + '</div>').css({
            position: 'absolute',
            display: 'none',
            padding: '2px',
            opacity: 0.80
        });

        var wWidth = $(window).width();
        var wHeight = windowHeight();

        // TODO make this work for tooltips wider that 250px
        // This method is a bit of a hack, but I had
        // problems getting JQuery ui's $.position method
        // to flip correctly when it detected a collision.
        if (wWidth < (x + 250))
        {
            var pos = wWidth - x + 20;
            popup.css({ right: pos })
        }
        else
        {
            popup.css({ left: x + 5 })
        }

        if (wHeight < (y + 100))
        {
            popup.css({ bottom: wHeight - (y - 5) })
        }
        else
        {
            popup.css({ top: y + 5 })
        }

        popup.appendTo("body").fadeIn(200);
    }

});

function initGenerator(obj)
{
    obj.yMax = 0;
    obj.highWater = 0;
    obj.dayMs = 1000 * 60 * 60 * 24;
    obj.stopDate = null;
    obj.todayEnd = vCore.dateToDayEnd(new Date());
    obj.sprintCompleted = true;
    obj.displayTrend = true;

    // helper function to update yMax for each iterations data
    obj.updateYMax = function ()
    {
        var oldYMax = obj.yMax;
        for (var i = 0; i < arguments.length; i++)
        {
            if (arguments[i] > oldYMax)
            {
                obj.yMax = arguments[i];
                oldYMax = arguments[i];
            }
        }
    };

    obj.scaleTime = function (v)
    {
        var scaled = Math.round(v / 15.0) / 4.0;

        return (scaled);
    };
}

function HourBurndownGenerator(timeline, startDate, endDate)
{
    initGenerator(this);
    this.workingDays = vCore.workingDays(startDate, endDate);
    this.timeline = timeline || null;
    this.startDate = startDate || null;
    this.endDate = endDate || null;
    this.ticks = null;
    this.displayWorkRequired = true;
    this.displayProjectedCompletion = true;
    this.trendSlope = 0;
    this.finishDateEstimate = "NA";

    this._chartData =
	{
	    workdone: [],
	    workleft: [],
	    workper: [],
	    estimateaccuracy: [],

	    worktoday: [],
	    worklefttoday: [],
	    workpertoday: [],

	    guessworkper: [],
	    guessworkdone: [],
	    guessworkleft: [],

	    trend: []
	};

    this.currentDate = new Date(startDate.getTime());
    this.startingLogged = null;
    this.startingEst = null;

    this.init = function ()
    {
        this.calculateStopDate();
        this.calculateWorkLogged();

        if (this.displayTrend)
            this.calculateTrend();
        this.finishDateEstimate = this.calculateEstimatedFinishDate();
    };

    /**
    * Helper function to calculate the stop date
    * 
    * If the sprint isn't completed, it will also set sprintCompleted to false 
    */
    this.calculateStopDate = function ()
    {
        if (vCore.areSameDay(this.startDate, this.endDate) &&
			vCore.areSameDay(this.startDate, this.todayEnd))
        {
            this.stopDate = vCore.dateToDayStart(this.startDate);
            return;
        }

        if (vCore.areSameDay(this.endDate, this.todayEnd))
        {
            this.stopDate = vCore.dateToDayStart(this.endDate);
            return;
        }

        if (this.endDate.getTime() <= this.todayEnd.getTime())
        {
            this.stopDate = vCore.dateToDayEnd(this.endDate);
            return;
        }

        if (this.startDate.getTime() > this.todayEnd.getTime())
        {
            this.stopDate = vCore.dateToDayStart(this.startDate);
        }
        else
        {
            this.stopDate = new Date(this.todayEnd.getTime());
            this.stopDate.setDate(this.stopDate.getDate() - 1);
        }
        this.sprintCompleted = false;

    };

    this.calculateWorkLogged = function ()
    {
        // Get the work already logged before the start of the sprint
        // This is to ensure that the "worked logged" line starts at
        // the origin at the beginning of the sprint.
        this.startingLogged = +this.timeline.getLoggedHours(vCore.dateToDayStart(this.startDate));
        this.startingEst = (+this.timeline.getEstimatedHours(vCore.dateToDayStart(this.startDate))) - this.startingLogged;
        this.highWater = this.startingEst;

        var first = true;
        var today = vCore.dateToDayEnd(new Date());
        var tickCount = this.ticks.length;

        var todaySeen = false;
        if (this.startDate.getTime() < today.getTime())
        {
            for (var i = 0; i < tickCount; i++)
            {
                var date = this.ticks[i];

                if (vCore.timestampAtDayEnd(date) <= this.stopDate.getTime())
                {
                    var estimate = this.timeline.getEstimatedHours(date) - this.startingLogged;
                    var logged = this.timeline.getLoggedHours(date) - this.startingLogged;
                    var remaining = estimate - logged;
                    if (remaining < 0)
                        remaining = 0;

                    if (remaining > this.highWater)
                        this.highWater = remaining;

                    var accuracy = logged + remaining;

                    var days = vCore.workingDays(date, this.endDate);

                    this._chartData.workdone.push(
							[i, this.scaleTime(logged)]);
                    this._chartData.workleft.push(
							[i, this.scaleTime(remaining)]);
                    this._chartData.estimateaccuracy.push(
							[i, this.scaleTime(accuracy)]);

                    if (this.displayWorkRequired)
                    {
                        if (days > 0)
                        {
                            this._chartData.workper.push(
								[i, this.scaleTime((remaining / days))]);
                        } else
                        {
                            this._chartData.workper.push(
								[i, this.scaleTime(remaining)]);
                        }
                    }

                    this.updateYMax(remaining, logged, accuracy);
                }
                else
                {
                    if (vCore.areSameDay(today, date))
                    {
                        if (!todaySeen && i > 0)
                        {
                            // Add a datapoint for yesterday so flot draws a line instead of a point
                            var yesterday = this.ticks[i - 1];
                            var yestimate = this.timeline.getEstimatedHours(yesterday) - this.startingLogged;
                            var ylogged = this.timeline.getLoggedHours(yesterday) - this.startingLogged;
                            var yremaining = yestimate - ylogged;
                            if (yremaining < 0)
                                yremaining = 0;

                            this._chartData.worktoday.push(
									[i - 1, this.scaleTime(ylogged)]);
                            this._chartData.worklefttoday.push(
									[i - 1, this.scaleTime(yremaining)]);

                            if (this.displayWorkRequired)
                            {
                                var ydays_left = vCore.workingDays(yesterday, this.endDate);

                                if (ydays_left > 0)
                                    this._chartData.guessworkper.push(
											[i - 1, this.scaleTime((yremaining / ydays_left))]);
                                else
                                    this._chartData.guessworkper.push(
											[i - 1, this.scaleTime(yremaining)]);
                            }
                        }

                        todaySeen = true;
                        // Plot a point for "today"
                        var estimate = this.timeline.getEstimatedHours(date) - this.startingLogged;
                        var logged = this.timeline.getLoggedHours(date) - this.startingLogged;
                        var remaining = estimate - logged;
                        if (remaining < 0)
                            remaining = 0;

                        var accuracy = logged + remaining;
                        if (remaining > this.highWater)
                            this.highWater = remaining;

                        this._chartData.worktoday.push(
								[i, this.scaleTime(logged)]);
                        this._chartData.worklefttoday.push(
								[i, this.scaleTime(remaining)]);
                        this._chartData.estimateaccuracy.push(
								[i, this.scaleTime(accuracy)]);

                        if (this.displayWorkRequired)
                        {
                            var days_left = vCore.workingDays(date, this.endDate);

                            if (days_left > 0)
                                this._chartData.guessworkper.push(
										[i, this.scaleTime((remaining / days_left))]);
                            else
                                this._chartData.guessworkper.push(
										[i, this.scaleTime(remaining)]);
                        }
                        this.updateYMax(remaining, logged, accuracy);

                        var days_elapsed = vCore.workingDays(this.startDate, date);
                        var estimate = this.timeline.getEstimatedHours(date) - this.startingLogged;
                        var logged = this.timeline.getLoggedHours(date) - this.startingLogged;
                        var remaining = estimate - logged;
                        if (remaining < 0)
                            remaining = 0;

                        var base = i > 4 ? i - 4 : 0;
                        var base_date = this.ticks[base];
                        var base_estimate = this.timeline.getEstimatedHours(base_date) - this.startingLogged;
                        var base_logged = this.timeline.getLoggedHours(base_date) - this.startingLogged;
                        var base_remaining = base_estimate - base_logged;
                        if (base_remaining < 0)
                            base_remaining = 0;

                        this.trendSlope = (base_remaining - remaining) / 4;
						
						// If this slope will intersect the x axis between ticks we 
						// adjust it so we ensure it intersects at the next tick
						var x_int = remaining / this.trendSlope;
						if (x_int % 1 != 0)
						{
							x_int = Math.ceil(x_int);
							this.trendSlope = remaining / x_int;
						}
                    }

                    this._chartData.guessworkleft.push(
							[i, this.scaleTime(remaining)]);

                    remaining -= this.trendSlope;


                    if (remaining < 0)
                        remaining = 0;
                }
            }
        }
        else
        {
            this._chartData.workdone.push(
					[0, this.scaleTime(0)]);
            this._chartData.workleft.push(
					[0, this.scaleTime(this.startingEst)]);
        }
    };
    this.calculateEstimatedFinishDate = function ()
    {
        var values = this._chartData.guessworkleft;
        if (!values)
            return "Never";

        var len = values.length;
        var last_rem = values[len - 1];
        var lastDate = null;

        if (this.trendSlope > 0)
        {
            if (last_rem[1] <= 0)
            {
                for (var i = 0; i < values.length; i++)
                {
                    lastDate = this.ticks[values[i][0]];
                    if (values[i][1] <= 0)
                    {
                        break;
                    }
                }
            }
            else
            {
                lastDate = vCore.dateToDayEnd(this.ticks[last_rem[0]]);
                var rem_working_days = Math.ceil(last_rem[1] / (this.trendSlope / 60));
                      
                for (var i = 0; i < rem_working_days; i++)
                {
                    lastDate = vCore.nextWorkingDay(lastDate);
                }

            }
        }
        if (!lastDate)
        {
            return "Never";
        }
        var curYear = new Date().getFullYear();
        if (curYear == lastDate.getFullYear())
        {
            return lastDate.strftime("%a %b %d");
        }

        return lastDate.strftime("%a %b %d, %Y");
    };
    this.calculateTrend = function ()
    {
        this._chartData.trend =
		[
			[0, this.scaleTime(this.highWater)],
			[this.ticks.length - 1, 0]
		]
    };

    this.chartData = function ()
    {
        var series = [
			{
			    label: "Effort",
			    data: this._chartData.workdone,
			    color: "darkblue",
				hoverable: true
			}, {
			    label: "Burndown",
			    data: this._chartData.workleft,
			    color: "green",
				hoverable: true
			}, {
			    data: this._chartData.worktoday,
			    color: "darkblue",
			    dashes: { show: true },
			    lines: { show: false },
				hoverable: true
			}, {
			    data: this._chartData.worklefttoday,
			    color: "green",
			    dashes: { show: true },
			    lines: { show: false },
				hoverable: true
			}, {
			    label: "Total Estimate",
			    data: this._chartData.estimateaccuracy,
			    color: "yellow",
				hoverable: true
			}];

        if (this.displayWorkRequired)
        {
            series.push({
                label: "Required rate",
                data: this._chartData.workper,
                color: "darkorange"
            });
            series.push({
                data: this._chartData.guessworkper,
                color: "orange",
                dashes: { show: true },
                lines: { show: false }
            });
        }

        if (this.displayProjectedCompletion)
        {
            series.push({
                data: this._chartData.guessworkleft,
                color: "lightgreen",
                dashes: { show: true, dashLength: 5 },
                points: { show: false },
                lines: { show: false }
            });
        }

        if (this.displayTrend)
        {
            series.push({
                data: this._chartData.trend,
                color: "red",
                lineWidth: 1,
                shadowSize: 0,
                points: { show: false }
            });
        }

        if (this.yMax == 0)
            this.yMax = this.highWater;

        var ret = {
            yMax: this.scaleTime(this.yMax),
            series: series
        };

        return ret;
    };

}


function WIBurndownGenerator(timeline, startDate, endDate)
{
    initGenerator(this);
    this.workingDays = vCore.workingDays(startDate, endDate);
    this.timeline = timeline || null;
    this.startDate = startDate || null;
    this.endDate = endDate || null;

    this._chartData =
	{
	    witopen: [],
	    witcreated: [],
	    witper: [],
	    guesswitper: [],

	    witopentoday: [],
	    witcreatedtoday: [],
	    witpertoday: [],

	    trend: []
	};

    this.currentDate = new Date(startDate.getTime());

    this.init = function ()
    {
        this.calculateStopDate();
        this.calculateWITProgress();

        if (this.displayTrend)
            this.calculateTrend();
    };

    this.calculateStopDate = function ()
    {
        if (this.endDate.getTime() > this.todayEnd.getTime())
        {
            this.stopDate = new Date(this.todayEnd.getTime());
            this.sprintCompleted = false;
        }
        else
            this.stopDate = new Date(this.endDate.getTime());
    }

    this.calculateWITProgress = function ()
    {
        var first = true;

        var today = vCore.dateToDayEnd(new Date());
        var tickCount = this.ticks.length;
        for (var i = 0; i < tickCount; i++)
        {
            var date = this.ticks[i];

            if (date.getTime() < this.stopDate.getTime())
            {
                var open = +this.timeline.getOpenWICount(date);
                var created = +this.timeline.getCreatedWICount(date);
                var closed = +this.timeline.getClosedWICount(date);
                var days = vCore.workingDays(date, this.endDate);
                var rate = Math.round(open / days * 100) / 100;

                if (open > this.highWater)
                    this.highWater = open;

                if (first)
                {
                    this._chartData.witopen.push([i, open]);
                    this._chartData.witcreated.push([i, created]);
                    this._chartData.witper.push([i, rate]);
                    first = false;
                }

                this._chartData.witopen.push([i, open]);
                this._chartData.witcreated.push([i, created]);
                this._chartData.witper.push([i, rate]);

                this.updateYMax(open, created);
            }
            else
            {
                if (vCore.areSameDay(today, date))
                {
                    if (i > 0)
                    {
                        // Add a datapoint for yesterday so flot draws a line instead of a point
                        var yesterday = this.ticks[i - 1];
                        var yopen = this.timeline.getOpenWICount(yesterday);
                        var ycreated = this.timeline.getCreatedWICount(yesterday);
                        var yclosed = this.timeline.getClosedWICount(yesterday);


                        this._chartData.witopentoday.push(
								[i - 1, yopen]);
                        this._chartData.witcreatedtoday.push(
								[i - 1, ycreated]);
                        //this._chartData.witclosedtoday.push( 
                        //		[ i - 1, yclosed ]);

                        var ydays_left = vCore.workingDays(yesterday, this.endDate);
                        var rate = Math.round(yopen / ydays_left * 100) / 100;
                        if (ydays_left > 0)
                            this._chartData.guesswitper.push(
									[i - 1, rate]);
                        else
                            this._chartData.guesswitper.push(
									[i - 1, yopen]);
                    }

                    var open = +this.timeline.getOpenWICount(date);
                    var created = +this.timeline.getCreatedWICount(date);
                    //var closed = +this.timeline.getClosedWICount(date);
                    var days = vCore.workingDays(date, this.endDate);
                    var rate = Math.round(open / days * 100) / 100;

                    if (open > this.highWater)
                        this.highWater = open;

                    this._chartData.witopentoday.push([i, open]);
                    this._chartData.witcreatedtoday.push([i, created]);
                    this._chartData.guesswitper.push([i, rate]);

                    this.updateYMax(open, created);

                }
            }
        }
    };

    this.calculateTrend = function ()
    {
        this._chartData.trend = [
		    [0, this.highWater],
		    [this.ticks.length - 1, 0]
		];
    };

    this.chartData = function ()
    {
        var series = [
			{
			    label: "Required rate",
			    data: this._chartData.witper,
			    color: "darkorange"
			}, {
			    data: this._chartData.guesswitper,
			    color: "orange"
			}, {
			    label: "Created",
			    data: this._chartData.witcreated,
			    color: "darkblue"
			}, {
			    label: "Burndown",
			    data: this._chartData.witopen,
			    color: "green"
			}, {
			    data: this._chartData.witcreatedtoday,
			    color: "darkblue",
			    dashes: { show: true },
			    lines: { show: false }
			}, {
			    data: this._chartData.witopentoday,
			    color: "green",
			    dashes: { show: true },
			    lines: { show: false }
			}];

        if (this.displayTrend)
        {
            series.push({
                data: this._chartData.trend,
                color: "red",
                lineWidth: 1,
                shadowSize: 0,
                points: { show: false }
            });
        }

        if (this.yMax == 0)
            this.yMax = 1;

        var ret = {
            yMax: this.yMax,
            series: series
        };

        return ret;
    };
}

/*
* Object that represents the timeline of events in the lifetime of a sprint.
* Once populated with the burndown data, it is possible to retrieve a
* snapshot of what the sprint status was at a given date.
* 
* It accepts an optional parameter of a user id to filter the returned data to
* just that of the give user.
*/
function burndownTimeline(user)
{
    this.user = user || null;

    this.recids = [];
    this.rec_dates = {};
    this.store = {};
    this.firstEstimates = {};

    this.startDate = null;
    this.endDate = null;
    this.estimatedFinish = null;
    /*
    * Helper method to generate a textual key for the given date
    */
    function _dateKey(date)
    {
        return Math.round(date.getTime()) + "";
    }

    /*
    * Add a wit history record for the given date
    */
    this.addRec = function (record)
    {
        var recid = record["recid"];
        var date = new Date(record.date);

        if (!this.startDate)
        {
            this.startDate = new Date(date.getTime());
        }
        else
        {
            if (date.getTime() < this.startDate.getTime())
                this.startDate = new Date(date.getTime());
        }
        if (!this.endDate)
        {
            this.endDate = new Date(date.getTime());
        }
        else
        {
            if (date.getTime() > this.endDate.getTime())
                this.endDate = new Date(date.getTime());
        }

        var date_key = _dateKey(date);

        if ($.inArray(recid, this.recids) == -1)
            this.recids.push(recid);

        if (this.rec_dates[recid] == undefined)
            this.rec_dates[recid] = [];

        this.rec_dates[recid].push(date);
        this.rec_dates[recid].sort();

        if (this.store[date_key] == undefined)
            this.store[date_key] = {};


        if (this.store[date_key][recid])
            vCore.consoleLog("date collision");

        this.store[date_key][recid] = record;

        if (record.estimate == 0 || record.estimate == null || record.estimate == undefined)
            return;
    }

    /*
    * Get the requested record as it existed at the given date
    * 
    * If the record didn't exist at that date, null will be returned
    */
    this.getRecordAtDate = function (recid, date)
    {
        // Find the most recent change for this record
        // before the given date
        var foundDate = null;

        if (this.rec_dates[recid] == undefined)
            return null;

        for (var i = 0; i < this.rec_dates[recid].length; i++)
        {
            var rdate = this.rec_dates[recid][i];
            if (rdate <= date)
            {
                if ((foundDate == null) || (rdate >= foundDate))
                {
                    foundDate = rdate;
                }
            }
        }

        // The record didn't exist before `date`
        if (foundDate == null)
            return null;

        var date_key = _dateKey(foundDate);

        if (this.store[date_key] == undefined)
            return null;

        if (this.store[date_key][recid] == undefined)
            return null;

        return this.store[date_key][recid];
    }

    /*
    * Get the count of open work items at the given date
    */
    this.getOpenWICount = function (date)
    {
        var open = 0;
        for (var i in this.recids)
        {
            var recid = this.recids[i];
            var rec = this.getRecordAtDate(recid, date);

            if (rec == null)
                continue;

            if ((!!this.user) && ((!rec.assignee) || (rec.assignee != this.user)))
                continue;

            if (rec.status == "open")
                open += 1;
        }

        return open;
    }

    /*
    * Get the number of closed work items at the given date
    */
    this.getClosedWICount = function (date)
    {
        var closed = 0;
        for (var i in this.recids)
        {
            var recid = this.recids[i];
            var rec = this.getRecordAtDate(recid, date);

            if (rec == null)
                continue;

            if ((!!this.user) && ((!rec.assignee) || (rec.assignee != this.user)))
                continue;

            if (rec.status != "open")
                closed += 1;
        }

        return closed;
    }

    /*
    * Get the number of new work items that were created on the given date
    */
    this.getCreatedWICount = function (date)
    {
        var created = 0;

        dates = [date];

        // If the date is a monday get any workitems that were created over the
        // weekend as well
        if (date.getDay() == 1)
        {
            var _d = new Date(date.getTime());
            _d.setDate(_d.getDate() - 1);
            dates.push(new Date(_d.getTime()));
            _d.setDate(_d.getDate() - 1);
            dates.push(new Date(_d.getTime()));
        }

        for (var i = 0; i < dates.length; i++)
        {
            var date = dates[i];
            for (var j in this.recids)
            {
                var recid = this.recids[j];
                var rec = this.getRecordAtDate(recid, date);

                if (rec == null)
                    continue;

                if ((!!this.user) && ((!rec.reporter) || (rec.reporter != this.user)))
                    continue;

                var last = this.rec_dates[recid].length - 1;
                var recdate = this.rec_dates[recid][last];

                if (vCore.areSameDay(recdate, date))
                    created += 1;
            }
        }
        return created;
    }

    /*
    * Get the total number of work items
    */
    this.getTotalWICount = function (date)
    {
        if (!user) { return this.recids.length; }

        var total = 0;
        for (var i in this.recids)
        {
            var recid = this.recids[i];
            var rec = this.getRecordAtDate(recid, date);

            if (rec == null)
                continue;

            if ((!!this.user) && ((!rec.assignee) || (rec.assignee != this.user)))
                continue;

            total += 1;
        }

        return total;
    }

    /*
    * Get the total number of estimated hours at the given date
    */
    this.getEstimatedHours = function (date)
    {
        var estimate = 0;

        for (var i in this.recids)
        {
            var recid = this.recids[i];
            var rec = this.getRecordAtDate(recid, date);

            if (rec == null)
                continue;

            if ((!!this.user) && ((!rec.assignee) || (rec.assignee != this.user)))
                continue;

            estimate += parseInt(rec.estimate || 0);
        }

        return estimate;
    }

    this.getFirstEstimateForRecid = function (recid)
    {
        var tself = this;
        var start = this.startDate.getTime();

        if (!tself.firstEstimates[recid])
        {
            var d = null
            for (var j = 0; j < tself.rec_dates[recid].length; j++)
            {
                var _date = tself.rec_dates[recid][j];
                var rec = tself.getRecordAtDate(recid, _date);
                if (!rec.estimate)
                    continue;

                if (!d)
                {
                    d = new Date(_date.getTime());
                    continue;
                }


                var ediff = start - d.getTime();
                var ndiff = start - _date.getTime();

                if (Math.abs(ndiff) < Math.abs(ediff))
                    d = new Date(_date.getTime());
            }

            if (d && d.getTime() < this.startDate.getTime())
                d = new Date(this.startDate.getTime());

            // A item might not have a first estimate yet
            if (d)
                tself.firstEstimates[recid] = [d.getTime(), tself.getRecordAtDate(recid, d)];
        }

        return tself.firstEstimates[recid];
    };

    this.getFirstEstimate = function ()
    {
        var tself = this;
        var est = 0;
        $.each(this.recids, function (i, recid)
        {
            var rec = tself.getFirstEstimateForRecid(recid);

            if (!rec)
                return

            if ((!!tself.user) && ((!rec[1].assignee) || (rec[1].assignee != tself.user)))
                return;

            if (rec[1].estimate == 0 || rec[1].estimate == null || rec[1].estimate == undefined)
                return;

            var amt = 0;
            var date = new Date(rec[0])
            if (tself.recids.length > 1)
                var amt = parseInt(rec[1].logged_work) || 0;

            est += (parseInt(rec[1].estimate) - amt);
        });

        return est;
    };

    this.getEarliestFirstEstimate = function ()
    {
        var tself = this;
        var date = null;
        $.each(this.recids, function (i, recid)
        {
            var rec = tself.getFirstEstimateForRecid(recid);
            if (!rec)
                return;

            if (!date)
                date = rec[0]
            else
                date = date > rec[0] ? rec[0] : date;
        });

        return date;
    };

    /*
    * Get the sum of the work logged at a given date
    */
    this.getLoggedHours = function (date)
    {
        var logged = 0;

        for (var i in this.recids)
        {
            var recid = this.recids[i];
            var rec = this.getRecordAtDate(recid, date);

            if (rec == null)
                continue;

            if ((!!this.user) && ((!rec.assignee) || (rec.assignee != this.user)))
                continue;

            logged += parseInt(rec.logged_work) || 0;
        }

        return logged;
    }
}
