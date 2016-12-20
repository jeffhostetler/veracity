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

var workFormController = function (options)
{
    this.options = $.extend(true, {
        afterUpdate: null
    }, (options || {}));

    this.progress = null;
    this.form = null;
    this.item = null;
    this.savingIndicator = null
    this.enabled = true;
    this.originalEst = null;
    this.work = [];
    this.container = null;
    this.burndownDiv = null;
    this.modalTimePopup = null;
    this.saving = false;
    this.witOpen = true;

}

extend(workFormController, {

    setWit: function (wit)
    {
        this.item = wit;

        if (this.item.recid)
        {
            this.modalTimePopup.recid = this.item.recid;
            //            this.initBurndown(this.item.recid);
        }

        var workRec = { rectype: "work", when: new Date().getTime(), estimate: this.item.estimate };
        this.form.setRecord(workRec);
        this.originalEst = workRec.estimate;
        this.form.ready(true);
        this.form.ready();

        this.hideShowFields();
    },

    setWitForFilters: function (wit)
    {
        this.form.ready(false);
        this.item = wit;

        var workRec = { rectype: "work", when: new Date().getTime(), estimate: this.item.estimate };

        this.form.setRecord(workRec);
        this.originalEst = workRec.estimate;
        this.work = wit.work || [];
        this.form.ready(true);
        this.statusDidChange(wit.status);
        this.form.getField("amount").data("recid", this.item.recid);
        this.form.getField("estimate").data("recid", this.item.recid);

        this.hideShowFields();
    },
    renderAmount: function (formdiv, label, checkEstimate)
    {
        var self = this;

        var amtOpts = {
            disableable: false,
            addClass: 'innercontainer',
            validators: {
                custom: function (value)
                {
                    if (!vTimeInterval.validateInput(value))
                    {
                        var err = "Unable to parse this time interval.  Here are some " +
						"example valid intervals:\n 1 hour 15 minutes\n1d 2h 15m";
                        return err;
                    }
                    var parsed = vTimeInterval.parse(value);
                    if (parsed > 1073741820)
                    {
                        var err = "That's quite a few hours you're logging there. " +
						"Perhaps is time for some time off, a hobby or two, maybe " +
						"a vacation.  I hear the carribean is nice this time of year. " +
						"If you are going to insist on logging all that work at least " +
						"make it fewer than 17895697 hours";
                        return err;
                    }

                    if (parsed == 0)
                    {
                        var err = "Cannot log work for intervals of less that on minute";
                        return err;
                    }

                    if (checkEstimate)
                    {
                        var tmpWork = 0;
                        $.each(self.work, function (i, v)
                        {
                            tmpWork += parseInt(v.amount);
                        });
                        var formattedVal = parsed + tmpWork;

                        if (formattedVal > 1073741820)
                        {
                            var err = "Total work amount must be less than 17895697 hours";
                            return err;
                        }

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
            },
            onchange: function (input) { self.amtOnChange(input); }
        };

        if (label)
        {
            amtOpts.label = label;
        }
        var amount = this.form.field(formdiv, "text", "amount", amtOpts);

        amount.field.attr("id", "work_amount");

        amount.field.bind("keydown", function (event)
        {
            if (event.keyCode != 13)
                return;

            var amtField = self.form.getField("amount");
            if (amtField.val() == null)
                return;

            if (!$.browser.msie)
                amtField.field.trigger("change");

            self.form.submitForm();
        });

        this.amountFieldWatcher(amount);

        return amount;

    },
    renderEstimate: function (formdiv, showreader, label, checkForNullAmt)
    {
        var self = this;

        var estFormatter = function (val, method)
        {
            var work = 0;
            if (self.work)
            {
                if (checkForNullAmt)
                {
                    $.each(self.work, function (i, v)
                    {
                        if (v.amount != null)
                        {
                            work += parseInt(v.amount);
                        }
                    });
                }
                else
                {
                    $.each(self.work, function (i, v) { work += parseInt(v.amount); });
                }
            }
            var amountField = self.form.getField("amount");
            var amount = self.form.getFieldValue(amountField);

            if (method == "get")
            {
                if (val == "Complete")
                {
                    var estfield = self.form.getField("estimate");
                    var lastVal = estField.data("lastval");
                    val = (lastVal == null || lastVal == undefined || lastVal == NaN) ? lastVal : 0;
                }

                var estWork = vTimeInterval.parse(val);
                return estWork + work + amount;
            }
            else
            {
                if (self.item)
                    var recid = self.item.recid;

                if (val == "" || val == undefined || val == null)
                {
                    val = 0;
                }

                if (!recid)
                {
                    var zeroVal = "";
                }
                else
                {
                    var zeroVal = (work == 0 && val == 0 && (amount == 0 || amount == null)) ? "" : "Complete";
                }

                var adjWork = (val - work - amount);
                if (adjWork < 0)  // correct for a bug that has work logged but no estimate
                    adjWork = 0;

                return vTimeInterval.format(adjWork, zeroVal);
            }
        };

        var estDiv = $("<div>").attr("id", "wiWorkEstimate");

        var estOpts = {
            onchange: function (input)
            {
                if (self.form._ready)
                {
                    // self.estVals.push(estField.getVal());
                    self.updateTimeLogged();
                }
            },

            validators: {
                minVal: 0,
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
        if (showreader)
        {
            estOpts.reader = {
                hideLabel: true,
                formatter: function (val)
                {
                    if (val == "")
                    {
                        if (self.witOpen)
                            return "No Estimate";
                        else
                            return "Complete";
                    }
                    if (val == "Complete")
                        return val

                    return val + " remaining";
                }
            };

        }
        if (label)
        {
            estOpts.label = label;
        }
        var estField = this.form.field(estDiv, "text", "estimate", estOpts);

        estField.field.focus(function (event) { event.target.select(); }).mouseup(function (event) { event.preventDefault(); });

        //update the original estimate value if user changes the estimate
        //so clearing the work field works as expected
        estField.field.keyup(function (e)
        {
            var key = String.fromCharCode(e.keyCode);
            var regexp = /[\n\f\r\t\v]/g;
            var m = regexp.test(key);
            //don't set value on tab through etc. so we can go back to the correct user
            //entered estimate value if time worked field is cleared
            if (!m)
            {
                var v = vTimeInterval.parse($(this).val());
                if (v)
                {
                    self.originalEst = v;
                }
            }
        });
        return estDiv;
    },
    afterSubmitForm: function (form, record)
    {
        var self = this;
        this.work.push(record);
        this.clearSaving();
        this.form.record = { when: new Date().getTime(), estimate: record.estimate };
        this.updateTimeLogged();
        if (record.estimate > 0)
        {
            this.progress.show();
        }

        if (this.options.afterUpdate)
            this.options.afterUpdate(self.item.recid);

        this.saving = false;

    },
    createWorkForm: function (addProgress, submitParentOnly)
    {
        var self = this;

        var workOpts = {
            rectype: "work",
            method: "PUT",
            prettyReader: true,
            formKey: "work",
            errorFormatter: bubbleErrorFormatter,
            beforeSubmit: function (vform)
            {
                // Abort the save if the amount field is empty
                if (!vform.master)
                {
                    var amtField = vform.getField("amount");
                    if (amtField.val() == null)
                        return false;
                }

                self.saving = true;
            },
            beforeFilter: function (vform, rec)
            {
                rec.when = new Date().getTime();
                self.setSaving();
                return rec;
            },
            afterValidation: function (vform, valid)
            {
                if (!valid)
                    return;

                if (!vform.options.url)
                {
                    vform.options.url = vCore.repoRoot() + "/workitem/" + self.item.recid + "/work.json";
                }

            },
            afterSubmit: function (form, record)
            {
                self.afterSubmitForm(form, record);
            }
        };

        var workRec = { rectype: "work", when: new Date().getTime() };
        if (submitParentOnly)
        {
            workOpts.submitParentOnly = true;
        }
        workOpts.forceValueSet = true;
        this.form = new vForm(workOpts);
        this.form.setRecord(workRec);

        var workDiv = $("<div>").attr("id", "wiWork");
        this.container = workDiv;

        var progressDiv = $("<div>").attr("id", "wiWorkSaving");
        this.savingIndicator = new smallProgressIndicator("savingWork", "");
        progressDiv.append(this.savingIndicator.draw());
        this.savingIndicator.hide();

        workDiv.append(progressDiv);

        var formdiv = $("<div>").attr("id", "wiWorkForm");

        var when = this.form.hiddenField("when", {
            formatter: function (val, method)
            {
                if (method == "get")
                    return parseFloat(val);
                else
                    return val;
            }
        });

        formdiv.append(when);

        if (addProgress)
        {
            this.progress = new progressBar("wi");

            formdiv.append(this.progress.render());
        }

        return formdiv;

    },
    render: function ()
    {
        var self = this;

        var formdiv = this.createWorkForm().addClass('editable');
        var workDiv = this.container;

        this.modalTimePopup = new logWorkControl(null, {
            onClose: function ()
            {
                var amtField = self.form.getField("amount");
                amtField.reset();
                var estField = self.form.getField("estimate");
                estField.reset();
                self.container.show();
            },
            afterSave: function (form, record)
            {
                var rec = form.getSerializedFieldValues();

                self.work = [];
                if (rec.work)
                {
                    $.each(rec.work, function (i, wrec)
                    {
                        if (!wrec._delete)
                            self.work.push(wrec);
                    });
                }
                var est = rec.estimate;
                var estField = self.form.getField("estimate");
                estField.setVal(est);
                if (self.options.afterUpdate)
                    self.options.afterUpdate(rec.recid);
            }
        });

        this.addButton = this.form.submitButton("Add", { disableable: false }).attr("id", "work_submit");
        this.addButton.addClass("wiWorkSubmit");

        var amount = this.renderAmount(formdiv, "Time Worked");

        var bc = $('<div class="buttoncontainer" id="addcontainer"></div>').append(this.addButton);
        formdiv.append(bc);

        this.progress = new progressBar("wi");

        formdiv.append(this.progress.render());

        var estDiv = this.renderEstimate(formdiv, true, "Remaining Estimate");

        this.form.ready();

        workDiv.append(formdiv);
        workDiv.append(estDiv);

        this.burndownDiv = $("<div>").attr("id", "burndown");
        workDiv.append(this.burndownDiv);

        return workDiv;
    },

    amountFieldWatcher: function (field)
    {
        if (!this.enabled)
            return;

        var self = this;

        var checkField = function ()
        {
            var val = field.val();

            if (vTimeInterval.validateInput(val))
                _enableButton(self.addButton);
            else
                _disableButton(self.addButton);

            window.setTimeout(checkField, 250)
        }

        window.setTimeout(checkField, 250);
    },

    zeroWorkRemaining: function ()
    {
        var self = this;
        var rec = this.form.getFieldValues();
        var _log = 0;

        if (this.work.length > 0)
            $.each(this.work, function (i, v) { _log += parseInt(v.amount); });

        var amountField = this.form.getField("amount");
        var amt = amountField.getVal();

        var est = +(rec.estimate || 0);

        var logged = _log + amt;

        var estField = this.form.getField("estimate");
        estField.setVal(logged, false);

        this.updateTimeLogged();
    },

    statusDidChange: function (val)
    {
        if (val != "open")
        {
            this.witOpen = false;
            workController.zeroWorkRemaining();
            var estField = this.form.getField("estimate");
            estField.disable();
        }
        else
        {
            this.witOpen = true;
        }

        this.hideShowFields();
    },

    updateTimeLogged: function ()
    {
        var self = this;
        var rec = this.form.getFieldValues();
        var _log = 0;

        if (this.work.length > 0)
            $.each(this.work, function (i, v) { _log += parseInt(v.amount); });

        var amountField = this.form.getField("amount");
        var amt = amountField.getVal();

        var est = +(rec.estimate || 0);

        var logged = _log + amt;

        if (est == 0)
        {
            var sum = "No Estimate";
            var percent = 0;
        }
        else
        {
            var quantify = false;
            if ((est >= 60 && logged < 60))
                quantify = true;
            var complete = (logged >= 60) ? vTimeInterval.toHours(logged, quantify) : vTimeInterval.toMinutes(logged, quantify);
            var total = (est >= 60) ? vTimeInterval.toHours(est) : vTimeInterval.toMinutes(est);
            var sum = complete + " of " + total + " complete";
            var percent = Math.round((logged / est) * 100);
        }

        var logLink = $("<a>").attr({ title: "Work Overview", id: "wiTimeLink" }).text(sum);
        logLink.click(function (e)
        {
            e.preventDefault();
            var rec = self.form.getFieldValues();
            self.container.hide();
            var estField = self.form.getField("estimate");

            self.modalTimePopup.estimate = estField.field.data("lastval");
            self.modalTimePopup.amount = rec.amount;
            self.modalTimePopup.display();
        });

        this.progress.setSummary(logLink);
        this.progress.setPercentage(percent);

        //        if (this.item)
        //            this.initBurndown(this.item.recid);
    },

    amtOnChange: function (input)
    {
        if (this.saving || !this.form.validateInput(input))
            return;

        var amtField = this.form.getField("amount");
        var amt = amtField.val();

        var estField = this.form.getField("estimate");
        if (amt)
            estField.setVal(estField.val() - amt, false);
        else
        {
            estField.setVal(this.originalEst, false);
        }
        this.updateTimeLogged();
    },

    initBurndown: function (recid)
    {
        if (!recid || !this.burndownDiv)
            return;

        var self = this;
        this.burndownDiv.empty();
        this.burndownDiv.show();
        this.burndownDiv.append($("<label class='innerheading' />").text("Item Burndown"));
        var burndownCont = $("<div>").attr({ id: "burndownDiv" });
        this.burndownDiv.append(burndownCont);

        wiBurndownChart = new burndownChart(burndownCont, {
            displayPoints: false,
            displayLegend: false,
            displayTrend: false,
            displayWorkRequired: false,
            displayProjectedCompletion: false,
            startDate: "first_estimate",
            width: 194,
            height: 150,
            xLabelWidth: 25,
            dateFormat: "%m/%d"
        });

        wiBurndownChart.setLoading("Loading chart data");
        var url = vCore.getOption("currentRepoUrl") + "/burndown.json?type=item&recid=" + recid;

        vCore.ajax({
            url: url,
            dataType: 'json',
            success: function (data)
            {
                wiBurndownChart.hideLoading();

                // TODO find out why the json data is sometimes null
                if (data == null)
                {
                    vCore.consoleLog("Burndown data was null");
                    return;
                }

                var amtField = workController.form.getField("amount");
                var estField = workController.form.getField("estimate");

                if (amtField.isDirty() || estField.isDirty())
                {
                    var amt = amtField.val() || 0;
                    var _log = 0;

                    if (workController.work.length > 0)
                        $.each(workController.work, function (i, v) { _log += parseInt(v.amount); });

                    var rec = {
                        recid: recid,
                        date: new Date().getTime(),
                        status: "",
                        estimate: estField.val(),
                        logged_work: _log + parseInt(amt)
                    }
                    data.push(rec);
                }

                wiBurndownChart.setChartData(data);
                if (!wiBurndownChart.prepareChart())
                {
                    self.burndownDiv.hide();
                }
            },
            error: function (xhr, textStatus, errorThrown)
            {
                vCore.consoleLog("Error: Fetch Chart Data Failed");
                wiBurndownChart.displayError("Unable to display chart.");
            }
        });
    },

    setSaving: function ()
    {
        var height = $("#wiWork").height();
        $("#wiWorkSaving").css("height", height);

        $("#wiWorkForm").hide();
        $("#wiWorkSaving").show();
        this.savingIndicator.setText("Adding Work...");
        this.savingIndicator.show();
    },

    clearSaving: function ()
    {
        $("#wiWorkSaving").hide();
        $("#wiWorkForm").show();
        this.savingIndicator.hide();
    },

    disable: function ()
    {
        var self = this;

        $("#work_amount").bind("keydown", function (event)
        {
            if (event.keyCode != 13)
                return;

            var amtField = self.form.getField("amount");
            if (amtField.val() == null)
                return;

            if (!$.browser.msie)
                amtField.field.trigger("change");

            self.form.submitForm();
        });

        this.form.disable();
        this.enabled = false;
        this.hideShowFields();
        //        $("#work_amount").css("width", "163px")
    },

    enable: function ()
    {
        this.form.enable();
        this.enabled = true;
        this.hideShowFields();
        $("#work_amount").unbind("keydown");
        //        $("#work_amount").css("width", "205px")
    },

    hideShowFields: function ()
    {
        var rec = this.form.getFieldValues();
        var _log = 0;
        if (this.work.length > 0)
            $.each(this.work, function (i, v) { _log += parseInt(v.amount); });

        if (this.enabled)
        {
            if (this.witOpen)
            {
                this.form.getField("estimate").enable();
            }
            else
            {
                this.form.getField("estimate").disable();
            }
            this.form.getField("amount").show();
            $("#addcontainer").hide();
        }
        else
        {
            if (rec.estimate > 0)
            {
                this.form.getField("amount").show();
                if ($("#work_submit") && $("#work_submit").length)
                {
                    $("#addcontainer, #work_submit").show();
                }
            }
            else
            {
                this.progress.hide();
                this.form.getField("amount").hide();
                if ($("#work_submit") && $("#work_submit").length)
                {
                    $("#work_submit").hide();
                }
            }

            if (rec.estimate > 0 && rec.estimate == _log)
            {
                this.form.getField("estimate").disable();
            }
            else if (rec.estimate <= 0)
            {
                //                this.form.getField("estimate").label.hide();
            }

        }

    }

});
