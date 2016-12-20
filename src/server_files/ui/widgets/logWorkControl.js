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

function logWorkControl(witRecid, options)
{
	this.options = $.extend(true, {
		onClose: null,
		afterSave: null
	}, (options || {}));
	
	this.loadingIndicator = null;
	this.recid = witRecid;
	this.popup = null;
	this.popContent = null;
	this.wit = null;
	this.witHistory = {};
	this.work = [];
	this.form = null;
	this.itemForm = null;
	this.userLookup = null;
	this.ajaxQueue = null;
	this.inited = false;
	this.forms = [];
	this.toDelete = [];
	this.progress = null;
	this.estimate = null;
	this.amount = null;
	this.estimateField = null;
	this.curWork = 0;
	
	this._oldSubForms = [];
}

extend(logWorkControl, {

    /**
    * Initialize the logWorkControl
    *
    * This function does some initial setup of the control.  It is called by the
    * display() function and should not be called from outside of the control
    **/
    init: function ()
    {
        $("#logWorkControl").remove();
        this.popContent = $(document.createElement("div")).attr("id", "logWorkControl").addClass('outercontainer');


        //		$('#maincontent').prepend(this.popup.create(null, this.popContent));

        this.popup = this.popContent;
        var pu = this.popup;

        var popopts = {
            "id": "logWorkPopup",
            "autoOpen": false,
            "height": "auto",
            "minWidth": 750,
            "dialogClass": "primary",
            "width": 700,
            "resizable": false,
            "position": "center"
        };
        if (this.options.onClose)
            popopts.close = this.options.onClose;

        //		this.popup = new modalPopup("#backgroundPopup", null, popopts);

        this.popup.vvdialog(
			popopts
		);

        // TODO add loading code
    },

    fetchData: function ()
    {
        var lwself = this;

        this.ajaxQueue = new ajaxQueue({
            onSuccess: function () { lwself._fetchComplete(); },
            onError: function ()
            {
                //lwself._submitError();
            }
        });

        // Only fetch users if they haven't been given to us already

        if (!this.userLookup)
        {
            this.ajaxQueue.addQueueRequest({
                url: vCore.repoRoot() + "/users.json",
                dataType: "json",
                contentType: "application/json",
                type: "GET",
                success: function (data, status, xhr) { lwself.setUsers(data); }
            });
        }

        this.ajaxQueue.addQueueRequest({
            url: vCore.repoRoot() + "/workitem/" + this.recid + ".json?_fields=*,history",
            dataType: "json",
            contentType: "application/json",
            type: "GET",
            success: function (data, status, xhr) { lwself.setWit(data); }
        });

        this.ajaxQueue.addQueueRequest({
            url: vCore.repoRoot() + "/workitem/" + this.recid + ".json?_allVersions=true",
            dataType: "json",
            contentType: "application/json",
            type: "GET",
            success: function (data, status, xhr) { lwself.setWitHistory(data); }
        });

        this.ajaxQueue.addQueueRequest({
            url: vCore.repoRoot() + "/workitem/" + this.recid + "/work.json?_fields=*,history",
            dataType: "json",
            contentType: "application/json",
            type: "GET",
            success: function (data, status, xhr) { lwself.setWork(data); }
        });

        this.ajaxQueue.execute();
    },

    _fetchComplete: function ()
    {
        this.popContent.empty();

        // Find the original estimate
        this.orig_est = null;
        this.orig_date = null;
        for (var i = (this.wit.history.length - 1); i >= 0; i--)
        {
            var hist = this.wit.history[i];
            var hid = hist.hidrec;
            var rec = this.witHistory[hid];
            if (!rec)
                continue;


            var _est = rec.estimate;
            if (_est != undefined && _est != 0 && _est != null)
            {
                this.orig_date = new Date(parseInt(hist.audits[0].timestamp));
                this.orig_est = rec.estimate;
                break;
            }
        }

        this.draw();
    },

    setWit: function (wit)
    {
        this.wit = wit;
    },

    setWitHistory: function (hist)
    {
        var lwself = this;
        if (!$.isArray(hist))
            hist = [hist];

        this.witHistory = {};
        $.each(hist, function (i, rec)
        {
            lwself.witHistory[rec.hidrec] = rec;
        });
    },

    setWork: function (workRecs)
    {
        var lwself = this;
        this.work = [];
        this.curWork = 0;
        $.each(workRecs, function (i, v)
        {
            lwself.work.push(v);
            lwself.curWork += parseInt(v.amount);
        });
    },

    setUsers: function (data)
    {
        var lwself = this;
        this.userLookup = {};
        $.each(data, function (i, v) { lwself.userLookup[v.recid] = v; });
    },

    display: function ()
    {
        if (!this.inited)
            this.init();

        this.loadingIndicator = new smallProgressIndicator("workLoading", "Loading Existing Work...");
        this.popContent.append(this.loadingIndicator.draw());

        this.popup.vvdialog("open");

        this.fetchData();
    },

    hide: function ()
    {
        this.popup.vvdialog("vvclose");
    },

    _newWorkRow: function (amount)
    {
        if (!amount)
            amount = null;

        var lwself = this;
        var now = new Date();

        var newopts = {
            rectype: "work",
            prettyReader: true,
            multiRecord: true,
            url: vCore.repoRoot() + "/workitem/" + this.recid + "/work",
            method: "PUT",
            formKey: "work",
            errorFormatter: bubbleErrorFormatter
        };

        var newForm = new vForm(newopts);
        var rec = { rectype: "work", when: now.getTime(), amount: amount };
        newForm.setRecord(rec);

        var histitem = this.wit.history[this.wit.history.length - 1];

        var today = vCore.dateToDayEnd(now);
        if (this.orig_date)
            startDate = new Date(this.orig_date.getTime());
        else
            startDate = new Date(today.getTime());

        var newRow = $("<tr>").addClass("newWork");

        //User
        var user = this.userLookup[vCore.getOption("userName")];
        var userSpan = $("<span>").addClass("reader").text(user.name);
        var userCell = $("<td>").addClass("nameCell").append(userSpan).append(" worked");

        newForm.field(userCell, "hidden", "_delete", { reader: { hasReader: false} });
        newRow.append(userCell);

        // Amount
        var amtCell = $("<td>").addClass("amountCell");

        var amount = newForm.field(amtCell, "text", "amount", {
            addClass: "amount",
            reader: { tag: "span", "addClass": "no-ic" },
            validators: {
                required: true,
                custom: function (value)
                {
                    if (!vTimeInterval.validateInput(value))
                    {
                        var err = "Unable to parse this time interval.  Here are some " +
						"example valid intervals:\n 1 hour 15 minutes\n1d 2h 15m";
                        return err;
                    }

                    if (vTimeInterval.parse(value) > 1073741820)
                    {
                        var err = "That's quite a few hours you're logging there. " +
						"Perhaps is time for some time off, a hobby or two, maybe " +
						"a vacation.  I hear the carribean is nice this time of year. " +
						"If you are going to insist on logging all that work at least " +
						"make it fewer than 17895697 hours";
                        return err;
                    }

                    return true;
                }
            },
            formatter: function (val, method)
            {
                if (method == "get")
                    return vTimeInterval.parse(val);
                else
                    return vTimeInterval.format(val);
            }
        });

        amount.field.bind("keyup", function (e)
        {
            if (e.keyCode == 13)
            {
                lwself.addWorkClicked();
            }
            else if (e.keyCode == '27')
            {
                amount.setVal("");
                e.stopPropagation();
                e.preventDefault();
            }

        });

        newRow.append(amtCell);

        // Date
        var dateCell = $("<td>").addClass("whenCell").text("on ");
        var when = newForm.field(dateCell, "dateSelect", "when", {
            reader: { tag: "span", "addClass": "no-ic" },
            validators:
			{
			    required: true,
			    betweenDates:
				{
				    start: startDate,
				    end: today
				}
			}
        });

        when.field.bind("keyup", function (e)
        {
            if (e.keyCode == 13)
            {
                lwself.addWorkClicked();
            }
        });

        newRow.append(dateCell);
        var addButton = newForm.button("Add");
        addButton.click(function () { lwself.addWorkClicked(); });

        newRow.append($("<td>").addClass("submitCell").append(addButton));

        this.form = newForm;
        this.form.ready();
        return newRow;
    },

    draw: function ()
    {
        this.loadingIndicator.hide();
        var lwself = this;

        this.forms = [];

        var itemopts = {
            rectype: "item",
            multiRecord: true,
            url: vCore.repoRoot() + "/workitem-full/" + this.recid + "/work",
            method: "PUT",
            errorFormatter: bubbleErrorFormatter,
            queuedSave: false,
            prettyReader: true,
            beforeFilter: function (form, newRecord)
            {
                if (!newRecord.work)
                    return newRecord;

                var today = new Date();

                for (var i = 0; i < newRecord.work.length; i++)
                {
                    var workRec = newRecord.work[i];
                    if (workRec.recid)
                    {
                        workRec.when = parseInt(workRec.when);
                        continue;  // don't munge existing records
                    }
                    var d = new Date(workRec.when);

                    // If the work is being logged for today set the timestamp to now
                    if (vCore.areSameDay(today, d))
                        workRec.when = today.getTime();
                    else
                        workRec.when = vCore.timestampAtDayEnd(d); // Set the timestamp for work logged to the end of that day

                }

                return newRecord;
            },
            afterSubmit: function (form)
            {
                if (lwself.options.afterSave)
                    lwself.options.afterSave(form);

                lwself.popup.vvdialog("vvclose");

            }
        };

        this.itemForm = new vForm(itemopts);
        this.itemForm.setRecord(this.wit);
        var _csid = this.itemForm.hiddenField("_csid");
        var _recid = this.itemForm.hiddenField("recid");
        this.popContent.append(_csid);
        this.popContent.append(_recid);

        if (this.wit.title.length > 50)
            var title = "\"" + this.wit.title.substr(0, 50) + "...\"";
        else
            var title = this.wit.title;

        this.popup.vvdialog("option", "title", title + " Work Log");

        var tableHeader = $('<div class="innerheading"></div>');

        var workTable = $(document.createElement("table")).addClass("zebra");
        workTable.attr({
            "id": "workTable",
            "cellspacing": "0",
            "cellpadding": "0"
        });
        var tbody = $(document.createElement("tbody"));

        var newRow = this._newWorkRow(this.amount);
        tbody.append(newRow);

        var work = 0;

        var today = vCore.dateToDayEnd(new Date());
        var startDate = null
        if (this.orig_date)
            startDate = new Date(this.orig_date.getTime());
        else
            startDate = new Date(today.getTime());

        this.work.sort(function (a, b) { return (b.when - a.when); });
        $.each(this.work, function (i, item)
        {
            var editopts = {
                rectype: "work",
                prettyReader: true,
                multiRecord: true,
                url: vCore.repoRoot() + "/work/" + item.recid,
                method: "PUT",
                formKey: "work",
                errorFormatter: bubbleErrorFormatter,
                beforeSubmit: function (vform)
                {
                    var whenField = vform.getField("when");

                    var last = whenField.data("lastval");
                    var val = whenField.val();

                    if (vCore.areSameDay(new Date(last), new Date(val)))
                    {
                        whenField.field.datepicker("destroy");
                        whenField.setVal(last, true, false);
                    }
                }
            };

            var editForm = new vForm(editopts);
            editForm.setRecord(item);

            var row = $(document.createElement("tr")).addClass("workRow");

            /**
            * Name Cell
            **/
            var name = $(document.createElement("td")).addClass("nameCell");
            if (item.history && item.history.length > 0)
            {
                var hLength = item.history.length - 1;
                if (item.history[hLength].audits.length > 0)
                {
                    var who = item.history[hLength].audits[0].userid;
                    var user = lwself.userLookup[who];
                    var nameSpan = $(document.createElement("span")).addClass("reader").text(user.name);
                    name.append(nameSpan).append(" worked");
                }
            }

            editForm.field(name, "hidden", "_csid", { reader: { hasReader: false} });
            editForm.field(name, "hidden", "_delete", { reader: { hasReader: false} });
            editForm.field(name, "hidden", "recid", { reader: { hasReader: false} });

            row.append(name);

            /**
            * Amount Cell
            **/
            var amtCell = $(document.createElement("td")).addClass("amountCell");
            editForm.field(amtCell, "text", "amount", {
                addClass: "amount",
                reader: { tag: "span", "addClass": "no-ic" },
                validators: {
                    required: true,
                    custom: function (value)
                    {
                        if (!vTimeInterval.validateInput(value))
                        {
                            var err = "Unable to parse this time interval.  Here are some " +
							"example valid intervals:\n 1 hour 15 minutes\n1d 2h 15m";
                            return err;
                        }

                        if (vTimeInterval.parse(value) > 1073741820)
                        {
                            var err = "That's quite a few hours you're logging there. " +
							"Perhaps is time for some time off, a hobby or two, maybe " +
							"a vacation.  I hear the carribean is nice this time of year. " +
							"If you are going to insist on logging all that work at least " +
							"make it fewer than 17895697 hours";
                            return err;
                        }

                        return true;
                    }
                },
                formatter: function (val, method)
                {
                    if (method == "get")
                        return vTimeInterval.parse(val);
                    else
                        return vTimeInterval.format(val);
                }
            });

            row.append(amtCell);

            /**
            * When Cell
            **/
            var whenCell = $(document.createElement("td")).addClass("whenCell").text("on ");

            var _startDate = startDate.getTime() > parseFloat(item.when) ? new Date(item.when) : startDate;
            editForm.field(whenCell, "dateSelect", "when", {
                reader:
				{
				    tag: "span", "addClass": "no-ic"
				},
                validators:
				{
				    required: true,
				    numerical: true,
				    betweenDates:
					{
					    start: _startDate,
					    end: today
					}
				}
            });
            row.append(whenCell);

            /**
            * Submit Cell
            **/

            var submitCell = $(document.createElement("td")).addClass("submitCell");

            var editLink = $("<a>").addClass("edit").text("edit");
            editLink.click(function () { lwself.editClicked(editForm, row); });

            var deleteLink = $("<a>").addClass("delete").text("delete");
            deleteLink.click(function () { lwself.deleteClicked(editForm, row); });

            var done = editForm.button("Done").addClass("done").hide();
            done.click(function () { lwself.doneClicked(editForm, row); });
            submitCell.append(done);

            submitCell.append(editLink).append(deleteLink);
            row.append(submitCell);
			
            tbody.append(row);

            editForm.ready();
            editForm.disable();

            lwself.forms.push(editForm);
        });
        workTable.append(tbody);
        this.popContent.append(workTable);

        if (this.estimate)
            est = this.estimate;
        else
            var est = parseInt((this.wit.estimate || 0));


        if (est < this.curWork)
            est = this.curWork;

        /**
        * Render the progress bar
        **/
        this.progress = new progressBar("wi");
        var percent = Math.round((this.curWork / est) * 100);
        var complete = (this.curWork >= 60) ? vTimeInterval.toHours(this.curWork) : vTimeInterval.toMinutes(this.curWork);
        var total = (est >= 60) ? vTimeInterval.toHours(est) : vTimeInterval.toMinutes(est);

        var sum = complete + " of " + total + " complete";
        this.popContent.append(this.progress.render());

        this.progress.setSummary(sum);
        this.progress.setPercentage(percent);

        var estFormatter = function (val, method)
        {
            if (method == "get")
            {
                if (val == "Complete" || val == "Unestimated")
                    val = 0;

                var estWork = vTimeInterval.parse(val);
                if (estWork == null)
                    return null
                else
                    return estWork + lwself.curWork;
            }
            else
            {
                if (val == "" || val == undefined || val == null)
                {
                    var zeroVal = "Unestimated";
                    val = 0;
                }
                else
                    var zeroVal = (lwself.curWork == 0 && val == 0) ? "Unestimated" : "Complete";

                var adjWork = (val - lwself.curWork);
                if (adjWork < 0)  // correct for a bug that has work logged but no estimate
                    adjWork = 0;

                return vTimeInterval.format(adjWork, zeroVal);
            }
        };

        var estOptions = {
            _append: false,
            reader: {
                hideLabel: true
            },
            validators: {
                custom: function (value)
                {
                    var formattedVal = estFormatter(value, "get");

                    if (formattedVal == null)
                    {
                        var err = "Unable to parse this time interval.  Here are some " +
						"example valid intervals:\n 1 hour 15 minutes\n1d 2h 15m";
                        return err;
                    }

                    if (formattedVal > 1073741820)
                    {
                        var err = "Estimated work must be less than 17895697 hours";
                        return err;
                    }

                    return true;
                }
            },
            formatter: estFormatter
        };

        if (this.estimateField)
        {
            var estName = "proxy_estimate";
            estOptions.dontSubmit = true;
        }
        else
        {
            var estName = "estimate";
        }

        var estField = this.itemForm.textField(estName, estOptions).attr("id", "item_estimate_modal");
        estField.blur(function () { lwself.updateTimeLogged(); });
        estField.focus(function (event) { event.target.select(); });
        estField.mouseup(function (event) { event.preventDefault(); }); ;

        var errorFormatter = new bubbleErrorFormatter(this.itemForm);

        var estLabel = this.itemForm.label(estName, "Remaining").removeClass('innerheading');
        var estError = errorFormatter.createContainer(estField, estField.data("options"));
        var estReader = $(document.createElement("p"));
        estReader.addClass("reader");
        estReader.css({ "display": "none" });

        var vffield = new vFormField(this.itemForm, estLabel, estField, estReader, estError, estField.data("options"));
        this.itemForm.fields.push(vffield);

        var estDiv = $("<div>").attr("id", "estimate").addClass('clearfix');
        var leftCont = $("<div>").addClass("leftCont").append(estField).append(estReader).append(estLabel).append(estError);
        var rightCont = $("<div>").addClass("rightCont").text("Original Estimate: " + vTimeInterval.format(this.orig_est));

        estDiv.append(leftCont).append(rightCont);
        this.popContent.append(estDiv);

        var buttons = [];
        buttons.push(
				{
				    'text': "OK",
				    'click': function () { lwself.okClicked(); }
				}
			);
        buttons.push(
				{
				    'text': "Cancel",
				    'click': function () { lwself.cancelClicked(); }
				}
			);


        this.popup.vvdialog("option", "buttons", buttons);

        if (!this.itemForm._ready)
            this.itemForm.ready();

        if (this.estimate)
        {
            var est = this.estimate;
        }

        this.itemForm.setFieldValue(estField, est);
        if (this.wit.status != "open")
        {
            $("#item_estimate_modal").attr("disabled", "disabled");
        }
        this.popup.vvdialog("open");

        Zebra.refresh();

        this.popup.vvdialog("option", "position", "center");
    },

    addWorkClicked: function ()
    {
        var lwself = this;

        this.form.clearErrors();
        //force the form to be dirty if add is clicked
        this.form.options.forceDirty = true;
        if (!this.form.validate())
        {
            this.form.displayErrors();
            return;
        }
        this.forms.push(this.form);

        var oldForm = this.form;
        var oldRow = $(".newWork").first();
        var amtField = $(".amount", oldRow).first();
        var amt = this.itemForm.getFieldValue(amtField);
        amtField.data("lastval", amt);

        oldRow.removeClass("newWork").addClass("workRow");
        var submitCell = $(".submitCell", oldRow);
        submitCell.empty();

        // Add links to the submit cell
        var editLink = $("<a>").addClass("edit").text("edit");
        editLink.click(function () { lwself.editClicked(oldForm, oldRow); });

        var deleteLink = $("<a>").addClass("delete").text("delete");
        deleteLink.click(function () { lwself.deleteClicked(oldForm, oldRow); });
        submitCell.append(editLink).append(deleteLink);

        var done = this.form.button("Done").addClass("done").hide();
        done.click(function () { lwself.doneClicked(oldForm, oldRow); });
        submitCell.append(done);
        
        this.form.disable();

        // Sort the rows
        var rows = $("#workTable tbody > tr").get();
        rows.sort(function (a, b)
        {
            var dateAtxt = $(".whenCell span", a).text();
            var dateBtxt = $(".whenCell span", b).text();
            if (dateAtxt && dateBtxt)
            {
                try
                {
                    return (new Date(dateBtxt) - new Date(dateAtxt));
                }
                catch (e)
                {
                    //this should never happen
                    return 0;
                }
            }
            //this should never happen
            return 0;
        });

        $.each(rows, function (index, row)
        {
            $("#workTable").children('tbody').append(row);
        });

        // Add the new form row to the table
        var newRow = this._newWorkRow();

        $("#workTable").children('tbody').prepend(newRow);

        this.updateTimeLogged();

        Zebra.refresh();
    },

    updateTimeLogged: function ()
    {
        var lwself = this;
        var est = $("#item_estimate_modal");
        var workest = this.itemForm.getFieldValue(est);

        var work = 0;
        $.each(this.forms, function (i, form)
        {
            var rec = form.getFieldValues();
            if (!rec._delete)
                work += parseInt(rec.amount);
        });


        this.curWork = work;
        if (work > workest)
            workest = work;

        // This looks wrong but it's not.  We need to force the 
        // field formatter to run again with the new curWork value
        this.itemForm.setFieldValue(est, workest, false);

        if (this.estimateField)
            this.itemForm.setFieldValue(this.estimateField, workest, false);

        var percent = Math.round((work / workest) * 100);
        var complete = (work >= 60) ? vTimeInterval.toHours(work) : vTimeInterval.toMinutes(work);
        var total = (workest >= 60) ? vTimeInterval.toHours(workest) + " hours" : vTimeInterval.toMinutes(workest);
        var sum = complete + " of " + total + " complete";

        this.progress.setSummary(sum);
        this.progress.setPercentage(percent);

    },

    deleteClicked: function (form, row)
    {
        var delField = form.getField("_delete");
        delField.setVal("true", false);

        row.hide();
        //row.remove();

        this.updateTimeLogged();
        Zebra.refresh();
    },

    editClicked: function (form, row)
    {
        $(".edit, .delete", row).hide();
        $(".done", row).show();
        form.enable();

    },

    doneClicked: function (form, row)
    {
        form.clearErrors();
        if (!form.validate())
        {
            form.displayErrors();
            return;
        }

        this.updateTimeLogged();
        $(".edit, .delete", row).show();
        $(".done", row).hide();
        form.disable();
    },

    okClicked: function ()
    {
        var lwself = this;

        var amtField = this.form.getField("amount");

        if (amtField.getVal(true) != "")
        {
            if (!confirm("You have made changes to the add new work form.  Do you want to continue without adding those changes?"))
            {
                return;
            }
        }

        lwself.itemForm.subForms = [];
        $.each(this.forms, function (i, form)
        {
            lwself.itemForm.addSubForm(form);
        });

        if (lwself.itemForm.isDirty(true))
        {
            this.itemForm.submitForm();
        }
        else
            this.cancelClicked();

    },

    cancelClicked: function ()
    {
        this.popup.vvdialog("vvclose");
    }
});

