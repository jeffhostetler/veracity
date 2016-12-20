/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, softwaer
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

var currentFilters = null;
var filterControl = null;
var columnControl;
var hasInactiveUsers = false;
var hasReleasedMilestones = false;
var textBoxFilterName = null;
var form = null;
var tb = null;
var items = [];
var filterForm = null;
var workController = null;
var timeFormAdded = false;
var filter = {};
var columns = ["id", "title", "reporter", "priority"];
var groupedHash = {};
var countNoEst = 0;
var totalEst = 0;
var totalWorked = 0;
var filtername = "";
var sortHash = {};
var columnMenusSet = false;
var numResults = 100;
var numFetched = 0;
var gettingItems = false;
var allFetched = false;
var newQuery = false;
var startDisplayAt = 0;
var savedOpts = null;
var state = {};
var localRefresh = false;
var newFilter = false;
var wiQueue = [];
var updateQueue = [];
var scrollPosition = 0;
var newRecid = "";
var milestones = null;
var $filterdialog = null;
var saveForm = null;
var noRefresh = false;
var $columndialog = null;
var scrollBound = false;
var gotColumnValues = false;
var groupedDialog = null;
var criteriaChanged = false;
var clearOnReload = false;
var template = null;
var dropDownValues = {};
var itemsHash = {};
var numAdded = 0;
var sort = "priority #DESC";
var currentItem = null;
var currentAddText = null;
var editing = false;

function _showFilterForm(currentfilter)
{
    var criteria = {};
    if (currentfilter)
        criteria = currentfilter.criteria;
    if (!filterControl)
    {
        filterControl = new checkboxTree($("#filter"), criteria, "filter", { getFunction: getFilterValues, title: "Choose Criteria" });
    }
    else
    {
        if (criteria)
        {
            filterControl.setSelected(criteria);
        }
        filterControl.draw("filtercontrol", $filterdialog);
        $filterdialog.vvdialog("vvopen");
    }
}

function _resetResults()
{
    newQuery = true;
    allFetched = false;
    numFetched = 0;
    savedOpts = null;
    items = [];
    itemsHash = {};
    if (clearOnReload)
    {
        $("#filterresults").find("table.datatable").remove();
        $("#filterdesctext").text("");
        clearOnReload = false;
    }
    groupedHash = {};
    //$.bbq.pushState({}, 2)
    //$(window).trigger('hashchange');
    startDisplayAt = 0;
    numAdded = 0;
}

function _resetDisplay()
{
    newQuery = true;
    savedOpts = null;
    startDisplayAt = 0;
    numAdded = 0;
}

function addColumnsToGroupBy()
{

    var div = $("<div>");
    var ul = $("<ul>").addClass("groupby");

    allcolumns["none"] = new itemTableColumn("(don't group)");

    $.each(allcolumns, function (colval, v)
    {
        if (colval != "stamp" && colval != "id" && colval != "title" && colval != "description"
           && colval != "remaining_time" && colval != "estimate" &&
           colval != "logged_time" && colval != "last_timestamp" && colval != "created_date" && colval != "estimated")
        {
            if (v && colval)
            {
                var li = $("<li>").text(v.Name).attr("id", colval);

                li.click(function (e)
                {
                    groupedby = $(this).attr("id");
                    state["groupedby"] = groupedby;

                    filter.groupedby = groupedby;
                    if (!filterCriteriaStateChanged())
                    {
                        var f = $.deparam.fragment()["filter"] || getQueryStringVal("filter");
                        if (f)
                        {
                            state["filter"] = f;
                        }
                    }
                    clearOnReload = true;
                    $.bbq.pushState(state);

                    groupedDialog.vvdialog("vvclose");
                });

                ul.append(li);
            }
        }

    });
    if (!columnMenusSet)
    {
        div.prepend(ul);
        columnMenusSet = true;
        groupedDialog.html(div);
        enableLink($("#link_groupedby"), _groupbyClick);
    }

}
function _showGroupedByDialog()
{
    if (!groupedDialog)
    {
        groupedDialog = $('<div>')
		.vvdialog({
		    resizable: false,
		    buttons:
			{
			    "Close": function ()
			    {
			        $(this).vvdialog("vvclose");
			    }
			},
		    autoOpen: false,
		    title: "Group Items By",
		    close: function (event, ui)
		    {
		        $("#backgroundPopup").hide();
		    }
		});

    }
    groupedDialog.vvdialog("vvopen");
    if (!gotColumnValues)
    {
        groupedDialog.html("<div><div class='smallProgress'></div><div class='statusText'>loading columns...</div></div>");
        _getAllColumnValues(null, addColumnsToGroupBy);
    }
    else
    {
        addColumnsToGroupBy();
    }
}

function createUserSelect(label)
{
    var div = $("<div>").attr("id", "divEdit" + label);
    div.css("display", "none");
    $("#maincontent").append(div);
    var select = filterForm.field($("#divEdit" + label), "userSelect", label, {

        allowEmptySelection: true
    });
    div.addClass("divEditItem");
    return div;
}

function _statusDidChange(e)
{
    var input = filterForm.getField("status");
    var val = input.val();

    workController.statusDidChange(val);
}

function createFieldSelect(label, data)
{
    var div = $("<div>").attr("id", "divEdit" + label);
    div.css("display", "none");

    $("#maincontent").append(div);

    var selectOpts = {
        allowEmptySelection: false       
    };
    if (label == "status")
    {
        selectOpts.validators = { required: true };
        selectOpts.onchange = _statusDidChange;
        selectOpts.validators ={
            custom: function (value)
            {
                if ($("#item_status").val() == "open" && $("#item_milestone").find(":selected").data("released") == true)
                {
                    return "Cannot open an item in a released milestone.";
                }
                return true;

            }
        }
    }

    var select = filterForm.field($("#divEdit" + label), "select", label, selectOpts);
    $.each(data, function (i, v)
    {
        if (v != "")
        {
            select.addOption(v, v);
        }
    });

    div.addClass("divEditItem");
    return div;
}

function createSprintSelect()
{
    var div = $("<div>").attr("id", "divEditMilestone");
    div.css("display", "none");

    $("#maincontent").append(div);

    var milestoneField = filterForm.field($("#divEditMilestone"), "sprintSelect", "milestone", {

        nested: true,
        emptySelectionText: "Product Backlog",
        filter: function (val)
        {
            if (val)
            {
                if (val.recid == filterForm.record.milestone)
                    return true;
                else
                    return val.releasedate == undefined;
            }
            else
                return true;
        },
        validators: {
	        custom: function (value)
	        {
	            if ($("#item_status").val() == "open" && $("#item_milestone").find(":selected").data("released") == true)
	            {
	                return "Please choose a milestone that has not been released.";
	            }
	            return true;

	        }
	    }
    });
    div.addClass("divEditItem");
    return div;
}

function createEditTextBox(label)
{
    var maxLength = (label == "description" ? 16384 : 140);
    var req = (label == "title" ? true : false);
    var div = $("<div>").attr("id", "divEdit" + label);
    $("#maincontent").append(div);
    div.css("display", "none");
    var f = filterForm.field($("#divEdit" + label), "textarea", label, {
        multiline: true,
        addclass: "overflow-auto",
        validators: { maxLength: maxLength, liveValidation: true, required: req }
    });

    div.append(f);
    div.addClass("divEditItem");
    return div;
}

function createEditEstimateText()
{
    var div = $("<div>").attr("id", "divEditEstimate");
    $("#maincontent").append(div);
    div.css("display", "none");
    var estDiv = workController.renderEstimate($("#divEditEstimate"), false, null, true, true);
    div.append(estDiv);
    div.addClass("divEditItem");
    return div;
}

function createEditAmountText()
{
    var checkEst = false;
    if ($.inArray("remaining_time", columns) < 0)
    {
        checkEst = true;
    }
    var div = $("<div id='divEditAmount' class='divEditItem' style='display:none'></div>").
		appendTo($("#maincontent"));

	var f = workController.renderAmount(div, null, checkEst);
	$(f.field).removeClass('innercontainer');

    return div;
}


function _setColumns(template)
{
    allcolumns = {};
    $(".divEditItem").remove();
    filterForm.fields = [];
    workController.form.fields = [];
    for (var f in template.rectypes.item.fields)
    {
        var field = template.rectypes.item.fields[f];
        var label = field.form ? field.form.label : null;
        var values = [];
        var col = null;
        if (f == "id")
            label = "ID";
        if (label)
        {
            if (label != "ID" && label != "Created" && label != "Reporter")
            {
                if (field.constraints && field.constraints.allowed)
                {
                    var div = createFieldSelect(f, field.constraints.allowed);
                    col = new itemTableColumn(label, div);
                }
                else if (field.datatype == "userid")
                {
                    var div = createUserSelect(f);
                    col = new itemTableColumn(label, div);
                }
                else
                {
                    var div = createEditTextBox(f);
                    col = new itemTableColumn(label, div);
                }
            }
            else
            {
                col = new itemTableColumn(label);
            }
        }
        allcolumns[f] = col;
    }

    allcolumns["last_timestamp"] = new itemTableColumn("Last Modified");
    var div = createSprintSelect();
    allcolumns["milestone"] = new itemTableColumn("Milestone", div);
    div = createEditAmountText("logged_time");
    allcolumns["logged_time"] = new itemTableColumn("Time Logged", div);
    div = createEditEstimateText("remaining_time");
    allcolumns["remaining_time"] = new itemTableColumn("Remaining Estimate", div);
    allcolumns["estimated"] = new itemTableColumn("Estimated");

    //TODO figure out how to handle stamps inline
    allcolumns["stamp"] = new itemTableColumn("Stamps");

    div = $("<div>").attr("id", "divEditButtons").addClass("divEditItem");
    $("#maincontent").append(div);
    div.css("display", "none");

    var saveButton = filterForm.imageButton("imgButtonSaveEdits", sgStaticLinkRoot + "/img/save16.png");
    saveButton.addClass("filterSaveButton");
    saveButton.click(function (e)
    {
        _saveItem($(this).data("recid"));
    });

    var cancelButton = filterForm.imageButton("imgButtonCacnelEdits", sgStaticLinkRoot + "/img/cancel18.png");
    cancelButton.addClass("filterCancelButton");
    cancelButton.click(function (e)
    {
        _cancelFormEdits();
    });
    div.append(saveButton);
    div.append(cancelButton);

    gotColumnValues = true;

    $(".divEditItem").keydown(function (e)
    {
        if (e.which == 13 && e.ctrlKey)
        {
            e.preventDefault();
            e.stopPropagation();
            _saveItem($(e.target).data("recid"), true, $(e.target).attr("id"));
            return false;
        }
        else if (e.which == 13)
        {
            e.preventDefault();
            e.stopPropagation();
            _saveItem($(e.target).data("recid"));
        }
        else if (e.which == 27)
        {
            e.preventDefault();
            e.stopPropagation();
            _cancelFormEdits();
        }

    });
}

function _cancelFormEdits()
{
    $(".trEdit").hide();
    $(".trReader").show();
    filterForm.reset();
    workController.form.reset();
    $(".bubbleError").hide();
    editing = false;
    currentItem = null;
    currentAddText = null;

}
function _getNextItem(curRecid)
{
    var i = 0;

    for (i = 0; i < items.length; i++)
    {
        if (items[i].recid == curRecid)
        {
            if (items[i + 1])
            {
                return { "recid": items[i + 1].recid, "index": i + 1 };
            }
        }
    }
}

function _saveItem(recid, gotoNext, currentControlId)
{
    filterForm.options.url = sgCurrentRepoUrl + "/workitem-full/" + recid + ".json";
    var next = _getNextItem(recid);
    filterForm.options.afterSubmit = function (form, record)
    {
        _updateItemFromRecord(form, record);

        if (gotoNext && ($.inArray("logged_time", columns) < 0 && $.inArray("remaining_time", columns) < 0))
        {
            editRow(next, currentControlId);
        }

    };
    workController.form.options.afterSubmit = function (form, record)
    {
        workController.afterSubmitForm(form, record);
        _updateWork(workController.work, record.estimate, filterForm.getRecordValue("recid"));
        form.reset();
        if (gotoNext)
        {
            editRow(next, currentControlId);
        }
    };
    filterForm.submitForm();
}

function _getAllColumnValues(template, onCompleteFunction, resetForm)
{
    if (!gotColumnValues || resetForm)
    {
        if (template)
        {
            _setColumns(template);
            _displayItems();
        }
        else
        {

            vCore.ajax({
                url: sgCurrentRepoUrl + "/filtervalues.json",
                dataType: 'json',
                success: function (data, status, xhr)
                {
                    _setColumns(data.template);
                },
                error: function ()
                {
                    $('#wiLoading').hide();
                },
                complete: function ()
                {
                    enableLink($("#link_groupby"), _groupbyClick);
                    enableLink($("#link_columns"), _columnsClick);
                    if (onCompleteFunction)
                        onCompleteFunction();

                }
            });
        }
    }
    else
    {
        enableLink($("#link_groupby"), _groupbyClick);
        enableLink($("#link_columns"), _columnsClick);

        if (!newFilter)
        {
            _displayItems();
        }
    }
}

function setColumnsForDialog()
{
    var div = $('<div>').attr("id", "columndiv");
    var ul = $('<ul>').attr("id", "ulColumns");
    $.each(allcolumns, function (i, f)
    {
        if (f)
        {
            if (i != "id" && i != "title" && i != "none" && i != "estimated")
            {
                var li = $("<li>");
                var a = $("<a>").text(f.Name).addClass("checkboxLink");
                // var field = data.template.rectypes.item.fields[f];
                var cbx = $('<input>').attr({ name: "columns", type: "checkbox", value: i });
                if ($.inArray(i, columns) >= 0)
                {
                    cbx.attr("checked", "checked");
                }

                li.prepend(cbx);
                ul.append(li);

                a.click(function (e){
                    e.stopPropagation();
                    e.preventDefault();
                    cbx.click();
                });
                li.append(a);
            }
        }
    });
    div.append(ul);

    enableLink($("#link_columns"), _columnsClick);

    $columndialog.html(div);

}
function _showColumnsDialog(currentfilter, setupOnly)
{
    if ($("#columndiv"))
        $("#columndiv").remove();

    if (!$columndialog)
    {
        $columndialog = $('<div>')
		            .vvdialog({
//		                position: [$("#link_columns").offset().left, $("#link_columns").offset().top + 20],

		                resizable: false,
		                buttons:
                        {
                            "OK": function ()
                            {
                                var newcols = ["id", "title"];
                                var selected = $("input[name='columns']:checked");

                                //we need to persist the order of any current columns so...
                                //add the new values
                                $.each(selected, function (j, cbx)
                                {
                                    var col = $(cbx).val();
                                    var currentIndex = $.inArray(col, columns);
                                    if (currentIndex >= 0)
                                    {
                                        newcols.splice(currentIndex, 0, col);
                                    }
                                    else
                                    {
                                        newcols.push(col);
                                    }

                                });

                                columns = newcols;
                                // columns = columns.concat(newcols);
                                state["columns"] = columns.join(",");
                                filter.columns = state["columns"];
                                if (!filterCriteriaStateChanged())
                                {
                                    var f = getQueryStringVal("filter");
                                    if (f)
                                    {
                                        state["filter"] = f;
                                    }
                                }
                                localRefresh = true;
                                $.bbq.pushState(state);

                                $(this).vvdialog("vvclose");

                            },
                            "Cancel": function ()
                            {
                                $(this).vvdialog("vvclose");
                            }
                        },
		                close: function (event, ui)
		                {
		                    $("#backgroundPopup").hide();
		                },
		                autoOpen: false,
		                title: 'Choose Columns'
		            });

    }

    $columndialog.vvdialog("vvopen");
    if (!gotColumnValues)
    {
        $columndialog.html("<div><div class='smallProgress'></div><div class='statusText'>loading columns...</div></div>");
        _getAllColumnValues(null, setColumnsForDialog);
    }
    else
    {
        setColumnsForDialog();
    }
}

function _changeColumns(vals)
{
    columns = vals;
    _resetDisplay();
    _displayItems(null, true);
}

function addNewFilter(json)
{
    var url = sgCurrentRepoUrl + "/filters.json";
    utils_savingNow("Saving filter...");
    vCore.ajax(
	{
	    type: "POST",
	    url: url,
	    data: JSON.stringify(json),
	    success: function (data, status, xhr)
	    {
	        state = {};
	        filter.recid = data;
	        newRecid = data;

	        criteriaChanged = false;
	        filter.name = $("#textboxfiltername").val();
	        vCore.setPageTitle(filter.name);
	        _saveVisitedFilter(filter.name, newRecid);
	        _setFilterMenu();

	        noRefresh = true;
	        $.bbq.pushState({}, 2);

	        state["filter"] = newRecid;
	        $.bbq.pushState(state);
	        $(window).trigger('hashchange');
	        $("#filtername").text(filter.name);
	        filtername = filter.name;
	        enableLink($("#link_deletefilter"), _deleteFilterClick);
	        currentFilters.push(filter.name);
	        utils_doneSaving();
	    }
	});
}

function deleteFilter(recid)
{
    var url = sgCurrentRepoUrl + "/filter/" + recid + ".json";
    $('#maincontent').addClass('spinning');

    vCore.ajax(
	{
	    type: "DELETE",
	    url: url,
	    success: function (data, status, xhr)
	    {
	        //filter = data;

	        $("#filtername").text("");
	        _deleteFromFiltersMenu(recid, function () { window.location.href = $.param.querystring(sgCurrentRepoUrl + "/workitems.html", state); });

	        $('#maincontent').removeClass('spinning');
	        state = {};
	        if (filter && filter.criteria)
	        {
	            state = filter.criteria;
	        }
	        if (groupedby)
	        {
	            state["groupedby"] = groupedby;
	        }
	        if (sort)
	        {
	            state["sort"] = sort;
	        }
	        if (columns)
	        {
	            state["columns"] = columns.join(",");
	        }

	    }
	});

}


function updateFilter(recid, json)
{
    var url = sgCurrentRepoUrl + "/filter/" + recid + ".json";
    filter.recid = recid;
    utils_savingNow("Saving filter...");
    vCore.ajax(
	{
	    type: "PUT",
	    url: url,
	    data: JSON.stringify(json),
	    success: function (data, status, xhr)
	    {
	        state = {};
	        if (newRecid)
	        {
	            noRefresh = true;
	            $.bbq.pushState({}, 2);
	            state["filter"] = newRecid;
	            noRefresh = false;
	        }

	        $.bbq.pushState(state, 2);
	        $(window).trigger('hashchange');
	        criteriaChanged = false;
	        utils_doneSaving();
	    }
	});

}

function bindObservers(tr)
{
    var selector = tr || $("table.datatable tr:not(.filtersectionheader, .tr-newwi, .tablegroupsep)");

    enableRowHovers(false, tr);

    if (!scrollBound)
    {
        $(window).scroll(function ()
        {

            // Chrome 5.0.x will call this twice:
            // http://code.google.com/p/chromium/issues/detail?id=43958
            // The loadingHistory flag is here to work around that.

            // scrollPosition = $(window).scrollTop();
            if (!gettingItems && isScrollBottom() && !allFetched)
            {
                newQuery = false;
                _getItems();
                $(window).scrollTop(scrollPosition);
            }
        });
        scrollBound = true;
    }
    var tdrt = selector.find("td.remainingtime");
    tdrt.click(function (e)
    {
        var pos = $(this).offset();
        $("#edittime").css(
        {
            "left": (pos.left) + "px",
            "top": (pos.top + ($(this).height() - 22) / 2) + "px"

        });

        _hookUpClickEdit($(this), $("#edittime"), $("#textEstimate"), $("#buttonAddEstimate"), false);

    });
    var tdtl = selector.find("td.timelogged");
    tdtl.click(function (e)
    {
        var pos = $(this).offset();
        $("#edittimelogged").css(
		    {
		        "left": (pos.left) + "px",
		        "top": (pos.top + ($(this).height() - 22) / 2) + "px"
		    });

        _hookUpClickEdit($(this), $("#edittimelogged"), $("#textLoggedTime"), $("#buttonAddTime"), false);

    });

}

function _hookUpClickEdit(clicked, div, textField, button, showDefaultText)
{
    var tr = clicked.parent("tr");
    textField.val("");
    //$(this).css({ "width": $("#edittime").width() });
    if (showDefaultText)
    {
        var curText = clicked.text();
        textField.val(curText);
    }
    button.data("recid", tr.data("recid"));

    div.show();
    textField.focus();
}

function _updateTimeLoggged()
{
    var id = $("#buttonAddTime").data('recid');
    var value = $("#textLoggedTime").val();
    var val = _getTimeInterval(value);
    if (val == null)
        return;
    var tr = $("#tr" + id);
    var tdLogged = tr.find("td.timelogged");
    var currentloggedTime = parseInt(tdLogged.data("origval")) || 0;

    //verify that the total time logged doesn't exceed the limit
    if (!_validateTime(val + currentloggedTime))
        return;

    var item =
    {
        recid: id,
        json: {
            when: new Date().getTime(),
            amount: val
        },
        value: val,
        afterSuccess: _afterUpdateWork,
        url: sgCurrentRepoUrl + "/workitem/" + id + "/work.json"

    };
    updateQueue.push(item);
    tr = $("#tr" + item.recid);
    var td = tr.find("td.timelogged");
    td.text("updating...");
    _deQueueWorkItemUpdate();
    $("#edittimelogged").hide();
}

function _validateTime(value)
{
    if (value > 1073741820)
    {
        var err = "Estimate must be fewer than 17895697 hours.";
        reportError(err);
        return false;
    }
    return true;
}

function _getTimeInterval(value)
{
    if (!vTimeInterval.validateInput(value))
    {
        reportError("Unable to parse this time interval. Here are some example valid intervals:\n1 hour 15 minutes\n1d 2h 15m");
        return null;
    }
    var val = vTimeInterval.parse(value);
    if (!_validateTime(val))
    {
        return null;
    }
    return val;
}

function _updateTimeEstimate()
{
    var id = $("#buttonAddEstimate").data('recid');
    var tr = $("#tr" + id);
    var td = tr.find("td.remainingtime");
    var tdLogged = tr.find("td.timelogged");

    var loggedTime = parseInt(tdLogged.data("origval")) || 0;
    var value = $("#textEstimate").val();
    var val = _getTimeInterval(value);
    if (val == null)
        return;

    var totalEst = loggedTime + val;
    if (!_validateTime(totalEst))
        return;

    var item =
    {
        recid: id,
        json: { "estimate": totalEst },
        value: val,
        totalEst: totalEst,
        afterSuccess: _afterUpdateTimeEst,
        url: sgCurrentRepoUrl + "/workitem/" + id + ".json"

    };
    updateQueue.push(item);

    td.text("updating...");
    _deQueueWorkItemUpdate();
    $("#edittime").hide();
}

function updateCurrentItem(recid, values)
{
    for (var i = 0; i < items.length; i++)
    {
        if (items[i].recid == recid)
        {
            var item = items[i];
            $.each(values, function (j, val)
            {
                var field = val.field;
                var value = val.value;
                if (field == "logged_time")
                {
                    if (!item.work)
                        item.work = [];
                    item.work.push({ amount: value });

                    var tr = getRemainingTime(item);
                    var lt = getLoggedTime(item);
                    item["remainingTimeValue"] = tr;
                    item["loggedTimeValue"] = lt;
                }

                else if (field == "remaining_time")
                {
                    item["estimate"] = value;
                    var tr = getRemainingTime(item);
                    item["remainingTimeValue"] = tr;
                }
                else if (field == "milestone")
                {
                    item[field] = value;
                    item["milestonename"] = _getUserFriendlyValue("milestone", value);
                }
                else if (field == "assignee" || field == "verifier")
                {
                    item[field] = value;
                    item[field + "name"] = _getUserFriendlyValue(field, value);
                }
                else
                    item[field] = value;
            });
            return item;
        }
    }
}

function _afterUpdateTimeEst(updateitem)
{
    var tr = $("#tr" + updateitem.recid);
    var td = tr.find("td.remainingtime");


    //update the item in the list and recacalculate remaining time
    var item = updateCurrentItem(updateitem.recid, { "remaining_time": updateitem.totalEst });

    if (item.remainingTimeValue == 0)
    {
        var sum = getLoggedTime(item);
        td.text((item.work && item.work.length && sum ? "Complete" : ""));
    }
    else
    {
        td.text(vTimeInterval.format(item.remainingTimeValue));
    }

    if (allFetched)
    {
        _recalculateSummary();
    }
}

function _afterUpdateWork(updateitem)
{
    var tr = $("#tr" + updateitem.recid);
    var tdLogged = tr.find("td.timelogged");
    var tdEst = tr.find("td.remainingtime");

    var item = updateCurrentItem(updateitem.recid, { "logged_time": updateitem.value });

    //put new value in the data store
    tdLogged.data("origval", item.loggedTimeValue);
    tdLogged.text(vTimeInterval.format(item.loggedTimeValue));

    //if new time logged value is greater than the estimate update the
    //estimate to the logged value
    if (isNaN(item.estimate) || (item.estimate < item.loggedTimeValue))
    {
        var est =
        {
            recid: item.recid,
            json: { "estimate": item.loggedTimeValue },
            value: item.loggedTimeValue,
            afterSuccess: _afterUpdateTimeEst,
            url: sgCurrentRepoUrl + "/workitem/" + item.recid + ".json"
        };
        updateQueue.push(est);

        tdEst.text("updating...");
        _deQueueWorkItemUpdate();
    }
    else
    {
        if (item.remainingTimeValue == 0)
        {
            var sum = getLoggedTime(item);
            tdEst.text((item.work && item.work.length && sum ? "Complete" : ""));
        }
        else
        {
            tdEst.text(vTimeInterval.format(item.remainingTimeValue));
        }

        //add the new value to the table
        if (allFetched)
        {
            _recalculateSummary();
        }
    }
}

function _getUserFriendlyValue(field, val)
{
    var text = val;
    switch (field)
    {
        case "id":
            {
                //we need to send this page's token to the work item page for navigation
                var a = $('<a>').text(text).attr({ href: sgCurrentRepoUrl + "/workitem.html?recid=" + v.recid });
                td.append(a);
            }
            break;
        //todo use rectype for this somehow
        case "assignee":
        case "reporter":
        case "verifier":
            {
                var user = usersHash[val];
                if (user)
                    text = user.name;
                else
                    text = "(unassigned)";
            }
            break;
        case "logged_time":
        case "remaining_time":
            {
                text = vTimeInterval.format(parseInt(val));
            }
            break;

        case "milestone":
            {
                var sprint = sprintHash[text];
                if (sprint)
                {
                    text = sprint.name;
                }
                else
                {
                    text = "Product Backlog";
                }
            }
            break;

    }

    if (!text || text.lengh == 0)
    {
        text = "(none)";
    }
    return text;
}


function _getNewWorkItem(recid, group)
{
    var lurl = sgCurrentRepoUrl + '/workitem-full/' + recid + '.json';
    utils_savingNow("Updating list...");
    vCore.ajax(
    {
        url: lurl,
        type: 'GET',

        success: function (data)
        {
            _saveVisitedItem(data.id);

            var table = $("#tablefiltereditems");
            var lastTR = $(".tr" + group + ":last");
            var index = lastTR.index();
            data.stamp = data.stamps;
            var toIndex = findLastIndex(groupedby, group);
            items.splice(toIndex + 1, 0, data);
            itemsHash[data.recid] = data;
           
            var tr = _createTableRow(data, toIndex + 1, group);

            lastTR.after(tr);
            if (allFetched)
            {
                _appendToSummary(data);
                _setSummary(items.length, countNoEst, totalEst, totalWorked);
            }
            numAdded++;
            bindObservers(tr);
        },
        error: function ()
        {
            $('#addingwiprogress').hide();
        },
        complete: function ()
        {
            utils_doneSaving();
            $('#addingwiprogress').hide();
        }

    });
}


function getFilterValues()
{
    this.nodes = [];
    this.template = {};
    this.users = [];
    this.sprints = [];

    this.vals = {};

    var self = this;
    $filterdialog.html("<div><div class='smallProgress'></div><div class='statusText'>loading filter criteria...</div></div>");
    $filterdialog.vvdialog("vvopen");
    vCore.ajax({
        url: sgCurrentRepoUrl + "/filtervalues.json",
        dataType: 'json',
        success: function (data, status, xhr)
        {
            self.vals = data;
            self.users = data.users.sort(function (a, b)
            {
                if (!a.inactive && b.inactive)
                {
                    return -1;
                }
                else if (a.inactive && !b.inactive)
                {
                    return 1;
                }
                else
                {
                    return (a.name.compare(b.name));
                }

            });

            self.template = data.template;
            self.sprints = data.sprints;

            var created = null;

            for (var f in data.template.rectypes.item.fields)
            {
                var field = data.template.rectypes.item.fields[f];
				var label = f;

				if (field.form && field.form.label)
					label = field.form.label;

                var ff = new checkboxTreeNode(label, f);
                if (field.form && field.form.in_filter)
                {
                    if (f == "created_date")
                    {
                        created = new checkboxTreeNode("Created", "created_date");
                        created.addNode(new checkboxTreeNode("any time", "any", true, null, null, true));
                        $.each(timevals, function (i, v)
                        {
                            var n = new checkboxTreeNode(v, i, true);
                            created.addNode(n);

                        });

                    }
                    else
                    {
                        if (field.constraints && field.constraints.allowed)
                        {
                            if (f != "status")
                            {
                                ff.addNode(new checkboxTreeNode("(none)", "(none)", true));
                            }
                            $.each(field.constraints.allowed, function (i, v)
                            {
                                if (v)
                                {
                                    ff.addNode(new checkboxTreeNode(v, v, true));
                                }
                            });

                        }
                        if (field.datatype == "userid")
                        {
                            var current = new checkboxTreeNode("current user*", "currentuser", true);
                            current.realvalue = sgUserName;
                            ff.addNode(current);

                            if (f != "reporter")
                            {
                                ff.addNode(new checkboxTreeNode("(unassigned)", "(none)", true));
                            }
                            $.each(self.users, function (i, v)
                            {
                                if (v.recid != NOBODY)
                                {
                                    var name = v.name;
                                    if (v.inactive)
                                    {
                                        name += " (inactive)";
                                        hasInactiveUsers = true;
                                    }
                                    ff.addNode(new checkboxTreeNode(name, v.recid, true, null, null, false, v.inactive));
                                    //name, value, selectable, nodes, click, isdefault, isinactive
                                }
                            });
                        }

                        self.nodes.push(ff);
                    }
                }
            }
            self.sprints.sort(function (a, b)
            {
                return (a.startdate - b.startdate);
            });
			
            var slookup = {};

            $.each(data.sprints, function (i, s)
            {
                s.children = [];
                slookup[s.recid] = s;
            });
			
            var mile = new checkboxTreeNode("Milestone", "milestone");

            if (data.currentsprint && data.currentsprint.length > 0)
            {
				var _cursprint = slookup[data.currentsprint];
                var currentNode = new checkboxTreeNode("current milestone (" + _cursprint.name + ")*", "currentsprint", true);
                currentNode.realname = "milestone";
                currentNode.realvalue = data.currentsprint;
                mile.addNode(currentNode);
                if (!sgCurrentSprint)
                    sgCurrentSprint = data.currentsprint;
            }
            var backlog = new checkboxTreeNode("Product Backlog", "(none)", true);
            backlog.realname = "milestone";

            $.each(data.sprints, function (i, s)
            {
                if (s.parent)
                {
                    var parent = slookup[s.parent];

                    if (parent)
                        parent.children.push(s.recid);
                }
            });

            $.each(data.sprints, function (i, s)
            {
                if (!s.parent)
                {
                    createSprintNode(s, data.currentsprint, backlog, data.sprints);
                }

            });
            mile.addNode(backlog);
            self.nodes.push(mile);
            self.nodes.push(created);
            var lasttimestamp = new checkboxTreeNode("Last Modified", "last_timestamp");
            lasttimestamp.addNode(new checkboxTreeNode("any time", "any", true, null, null, true));
            $.each(timevals, function (i, v)
            {
                var n = new checkboxTreeNode(v, i, true);
                lasttimestamp.addNode(n);

            });
            self.nodes.push(lasttimestamp);

            var hasTimeEstimate = new checkboxTreeNode("Estimated", "estimated");
            hasTimeEstimate.addNode(new checkboxTreeNode("ignore", "ignore", true, null, null, true));
            hasTimeEstimate.addNode(new checkboxTreeNode("yes", "yes", true));
            hasTimeEstimate.addNode(new checkboxTreeNode("no", "no", true));
            self.nodes.push(hasTimeEstimate);
            filterControl.draw("filtercontrol", $filterdialog);

        },

        error: function ()
        {
            $("#filterCriteriaProgress").hide();
        }

    });

    return this.nodes;
}

function _deQueueWorkItem()
{
    if (wiQueue.length > 0)
    {
        if ($.browser.msie)
            sleep(500);

        var item = wiQueue.shift();
        var text = item.text;
        var val = item.val;
        var wi = item.wi;

        var top = text.offset().top;
        var left = text.offset().left + text.width() + 120;
        $('#addingwiprogress').css({ "top": top + 2, "left": left, "position": "absolute" });
        $('#addingwiprogress').show();
        var purl = sgCurrentRepoUrl + '/workitems.json';

        vCore.ajax(
        {
            url: purl,
            dataType: 'text',
            contentType: 'application/json',
            processData: false,
            type: 'POST',
            data: JSON.stringify(wi),

            success: function (data)
            {
                wi.recid = data;

                _getNewWorkItem(wi.recid, val);

            },
            complete: function ()
            {
                // _updateFilterSession(wi.recid);
                _deQueueWorkItem();

                $('#addingwiprogress').hide();
            }
        });
    }
    else
    {
        currentAddText = null;
        editing = false;   
    }
}

function _updateFilterSession(id)
{
    vCore.ajax(
        {
            url: sgCurrentRepoUrl + "/filteredworkitems/session.json",
            dataType: 'text',
            contentType: 'application/json',
            processData: false,
            type: 'POST',
            data: JSON.stringify({ recid: id })

        });

}
function _deQueueWorkItemUpdate()
{
    if (updateQueue.length > 0)
    {
        if ($.browser.msie)
            sleep(500);

        var item = updateQueue.shift();
        var recid = item.recid;

        var uurl = item.url;
        vCore.ajax(
		    {
		        url: uurl,
		        dataType: 'json',
		        contentType: 'application/json',
		        processData: false,
		        type: 'PUT',
		        data: JSON.stringify(item.json),

		        success: function ()
		        {
		            if (item.afterSuccess)
		            {
		                item.afterSuccess(item);
		            }
		        },
		        complete: function ()
		        {
		            _deQueueWorkItemUpdate();

		        }
		    }
	    );

    }
    else
    {
        currentItem = null;
        editing = false;
    }
}

//add a work item based on the filter criteria
//this assumes only one value for each criteria option (ie. assignee: user@domain.com)
function _addWorkItem(text, option, val)
{

    var wi = {
        rectype: 'item',
        title: text.val(),
        status: "open"
    };
    if (filter.criteria)
    {
        $.each(filter.criteria, function (i, v)
        {
            if (v == "currentsprint")
            {
                wi[i] = sgCurrentSprint;
            }
            else if (v == "currentuser")
            {
                wi[i] = sgUserName;
            }
            else if (v != "none" && v != "(none)" && v != "")
            {
                wi[i] = v;
            }
            if (i == "stamp")
            {
                wi["stamps"] = [{ "name": v, "rectype": "stamp", "_delete": ""}];
            }
        });

    }
    if (option && val != "none" && val != "(none)" && val != "")
    {
        wi[option] = val;
    }

    wiQueue.push({ "text": text, "val": val, "wi": wi });
    _deQueueWorkItem();

}

function _canAddItemBasedOnFilter()
{
    var b = true;
    if (filter.criteria)
    {
        $.each(filter.criteria, function (i, v)
        {
            if (v && v.split(',').length > 1)
            {
                if (i != groupedby)
                {
                    b = false;
                }
            }
        });
    }

    return b;
}

function saveURLToSession()
{
    var surl = window.location.href;
    var params2 = {};
    params2["maxrows"] = 100;
    params2["skip"] = 0;
    surl = $.param.querystring(surl, params2);

    _saveLastQuerySession(surl);

}

function _displayTableFooter(table, type, value, force)
{
    var bDisplay = _canAddItemBasedOnFilter();

    if (bDisplay)
    {
        bDisplay = false;
        switch (type)
        {
            case "status":
                {
                    if (value == "open")
                        bDisplay = true;
                    break;
                }
            case "assignee":
            case "sprint":
            case "milestone":
            case "priority":
            case "verifier":
                {
                    bDisplay = true;
                    break;
                }
            case "none":
                {
                    bDisplay = true;
                    break;
                }
        }

        if (bDisplay)
        {
            var tr = $("<tr>").addClass("tr-newwi");

            var tdlink = $("<td>").attr("colspan", columns.length);
            var tdform = $("<td>").attr("colspan", columns.length + 1);

            var hidden = $("<div>").addClass("divaddnewitem").hide();

            var inp = $(document.createElement('textarea')).attr("id", "textareaadd" + type).addClass("addNewItemText")
            inp.attr({ 'rows': 1 });
            inp.css({ "width": "40%", "margin-right": "5px" });

            var a = $("<a>").text("new work item").attr({ "title": "add a new work item", "href": "#" }).addClass("newwi");

            a.click(function (e)
            {
                _cancelFormEdits();
                editing = true;
                e.preventDefault();
                e.stopPropagation();
                tdlink.attr("colspan", 1);
                hidden.show();

                inp.focus();
                currentAddText = inp;
                inp.keydown(function (e1)
                {
                    if (e1.which == 13)
                    {
                        e1.preventDefault();
                        e1.stopPropagation();

                        if (jQuery.trim(inp.val()) != "")
                        {
                            _addWorkItem(inp, type, value);
                            inp.val("");
                        }
                    }
                    if (e1.which == 27)
                    {
                        tdlink.attr("colspan", 4);
                        inp.val("");
                        hidden.hide();

                        a.show();
                        currentAddText = null;

                    }
                });
                a.hide();
            });
            var button = $(document.createElement("input"));

            var submit = $("<input>").attr({ "type": "image", "title": "submit", "value": "submit", "src": sgStaticLinkRoot + "/img/save16.png" }).addClass("imgButton");
            submit.click(function (e)
            {
                e.preventDefault();
                e.stopPropagation();
                if (jQuery.trim(inp.val()) != "")
                {
                    _addWorkItem(inp, type, value);
                    inp.val("");
                    
                }
            });
            var cancel = $("<input>").attr({ "type": "image", "title": "cancel", "value": "cancel", "src": sgStaticLinkRoot + "/img/cancel18.png" }).addClass("imgButton");
            cancel.click(function (e)
            {
                e.preventDefault();
                e.stopPropagation();
                inp.val("");
                hidden.hide();
                tdlink.attr("colspan", columns.length);
                a.show();
                currentAddText = null;
            });
            hidden.append(inp);

            hidden.append(submit);
            hidden.append(cancel);
            tdlink.html(a);
            tdform.html(hidden);
            tr.append(tdlink);
            tr.append(tdform);

            table.append(tr);
        }
    }
    gettingItems = false;
}

function _hookUpColumnDrag(th, tableid)
{
    th.droppable(
                    {
                        deactivate: function (event, ui)
                        {
                            $(this).css({ "border-left": "0px" });
                        },
                        over: function (event, ui)
                        {
                            // get column index
                            var $targetTable = $(this).closest('table');

                            var index = $(this).index();

                            var pos = $(this).position();

                            $(this).css("border-left", "10px solid #cfdfae");

                            $('.colDest').css('left', (pos.left - ($('.colDest').height() / 2)));

                            $('.colDest-top').css('top', (pos.top - $('.colDest').height()));

                            $('.colDest').show();

                        },
                        out: function (event, ui)
                        {
                            $(this).css("border-left", "0px");
                            $('.colDest').hide();
                        },
                        drop: function (event, ui)
                        {
                            $(this).css("border-left", "0px");

                            var $table = $("table.datatable");

                            var $targetTable = $(this).closest('table');

                            var orig_index = $targetTable.data('drag_index');

                            var new_index = $(this).index();

                            $table.find('tr')
                                .each(
                                    function (index, row)
                                    {
                                        if ($(row).parent('thead').length)
                                        {
                                            $(row).find('th').eq(orig_index).insertBefore($(row).find('th').eq(new_index));

                                        }
                                        else
                                        {
                                            $(row).find('td').eq(orig_index).insertBefore($(row).find('td').eq(new_index));
                                        }

                                        $('.colDest').hide();
                                    }
                                );

                            var tbl = $table[0];
                            columns = [];
                            var headers = $(tbl).find("th");
                            $.each(headers, function (r, v)
                            {
                                var id = $(v).attr("id");
                                if (id)
                                {
                                    columns.push(id);
                                }
                            });
                            filter.columns = columns.join(",");
                            state["columns"] = filter.columns;
                            noRefresh = true;
                            if (!filterCriteriaStateChanged())
                            {
                                var f = getQueryStringVal("filter");
                                if (f)
                                {
                                    state["filter"] = f;
                                }
                            }
                            else
                            {
                                if (filtername)
                                {
                                    state["name"] = filtername;
                                }
                            }
                            $.bbq.pushState(state);
                            saveURLToSession();
                        }
                    }
                );
    th.draggable(
                    {
                        appendTo: 'body',
                        helper: 'clone',
                        start: function (event, ui)
                        {
                            _cancelFormEdits();
                            // get column index
                            var index = $(this).index();

                            // add column index to data store
                            $(this).closest('table').data({ 'drag_index': index });
                        }

                    }
                );


}

function _addGroupSep(table)
{
	var tr = $('<tr class="tablegroupsep"></tr>');
	tr.append($('<td> </td>'));
	tr.appendTo(table);
}

//table group type is the groupedby value
function _addTableGroup(table, type, key, lastType, lastKey, forceFooter)
{
    if (table.find(".trReader").length)
    {
        _displayTableFooter(table, lastType, lastKey, forceFooter);
		_addGroupSep(table);
    }
    var tr = $("<tr>").addClass("filtersectionheader dataheaderrow");
    var td = $("<th>").attr("colspan", columns.length + 2);

	// don't force uppercase on user-entered data
	if ($.inArray(type, ['reporter','assignee','verifier','milestone']) >= 0)
		tr.addClass('preservecase');

    if (groupedby != "none")
    {
		td.text(_getUserFriendlyValue(type, key));      
    }
    else
    {
        td.text("");
    }
    tr.append(td);
    table.append(tr);
    return tr;
}

function _addNewTable()
{
    var table = $("<table>").addClass("datatable bottom").attr("id", "tablefiltereditems");
    $("#filterresults").append(table);

    var thead = $("<thead>");
    var tr = $("<tr>");


    var thcontext = $("<th>").css("width", "15px");
    tr.append(thcontext);

    var splits = sort.split(" ");
    $.each(columns, function (i, c)
    {

        if (c != "estimate")
        {
            var th = $("<th>").attr("id", c).css("cursor", "pointer").addClass("th-draggable");

            if (c == "title")
            {
                th.css("width", "40%");
            }
            else if (c == "id")
            {
                th.css("width", "10%");
            }

            th.text(allcolumns[c].Name);

            _hookUpColumnDrag(th, "#tablefiltereditems");
            var bClickable = true;
            if (c == "remaining_time" || c == "logged_time" || c == "stamp")
            {
                th.css("cursor", "default");
                tr.append(th);
                bClickable = false;
            }
            if (bClickable)
            {
                th.click(function (e)
                {
                    _cancelFormEdits();
                    if (!gettingItems)
                    {
                        e.preventDefault();
                        e.stopPropagation();

                        var s = sortHash[c];

                        if (!s)
                        {
                            sort = c;
                            var dir = "#ASC";
                            var css = "asc";

                            if (c == "priority")
                            {
                                dir = "#DESC";
                                css = "desc";
                                sort = c + " #DESC";
                            }
                            sortHash[c] = dir;
                            var sortDir = $(this).find("span");
                            $(sortDir).addClass(css);
                        }
                        else
                        {
                            var dir = (sortHash[c] == "#ASC" ? "#DESC" : "#ASC");
                            sort = c;
                            if (dir == "#DESC")
                                sort = c + " " + dir;
                            var sortDir = $(this).find("span");
                            $(sortDir).addClass((dir == "#ASC" ? "asc" : "desc"));
                        }

                        filter.sort = sort;

                        state["sort"] = sort;

                        filter.groupedby = groupedby;
                        if (!filterCriteriaStateChanged())
                        {
                            var f = getQueryStringVal("filter");
                            if (f)
                            {
                                state["filter"] = f;
                            }
                        }
                        _resetResults();

                        $.bbq.pushState(state);
                        $(window).trigger('hashchange');

                    }

                });
            }
            var sortDir = $("<span>");
            sortDir.html("&#160;");
            if (splits[0] == c)
            {
                if (splits.length == 1)
                {
                    sortDir.addClass("asc");
                }

                else
                {
                    sortDir.addClass("desc");
                }
            }
            th.append(sortDir);
            tr.append(th);
        }

    });

	tr.append("<th></th>");

    thead.append(tr);
    table.append(thead);


    $("#filterresults").append(table);

    return table;
}

function sortByUser(a, b)
{
    var userA = usersHash[a];
    var userB = usersHash[b];
    if (!userA && !userB)
    {
        return 0;
    }
    if (userA && !userB)
    {
        return -1;
    }
    if (!userA && userB)
    {
        return 1;
    }

    return (userA.name.compare(userB.name));

}

function sortByMilestone(a, b)
{
    var sprintA = sprintHash[a];
    var sprintB = sprintHash[b];

    if (!sprintA && !sprintB)
    {
        return 0;
    }
    if (sprintA && !sprintB)
    {
        return -1;
    }
    if (!sprintA && sprintB)
    {
        return 1;
    }
    if ((sprintA.name.compare(sprintB.name) != 0) && (sprintA.startdate == sprintB.startdate))
    {
        return sprintA.name.compare(sprintB.name);
    }
    return (sprintA.startdate - sprintB.startdate);
}

function itemSortFunction(a, b)
{
    var parts = sort.split(" ");
    switch (parts[0])
    {
        case "assignee":
        case "reporter":
        case "verifier":
            {
                if (sort.indexOf("#DESC") >= 0)
                {
                    return sortByUser(b.item[parts[0]], a.item[parts[0]]);
                }
                return sortByUser(a.item[parts[0]], b.item[parts[0]]);
            }
            break;
        case "milestone":
            {
                if (sort.indexOf("#DESC") >= 0)
                {
                    return sortByMilestone(b.item["milestone"], a.item["milestone"]);
                }
                return sortByMilestone(a.item["milestone"], b.item["milestone"]);
            }
            break;

        case "remaining_time":
            {
                var a1 = parseInt(a.item["remainingTimeValue"]);
                var b1 = parseInt(b.item["remainingTimeValue"]);
                if (sort.indexOf("#DESC") >= 0)
                {
                    return b1 - a1;
                }
                return a1 - b1;
            }
            break;
        case "logged_time":
            {
                var a1 = parseInt(a.item["loggedTimeValue"]);
                var b1 = parseInt(b.item["loggedTimeValue"]);
                if (sort.indexOf("#DESC") >= 0)
                {
                    return b1 - a1;
                }
                return a1 - b1;
            }
            break;
    }
    return 0;
}

function sortItemKeys(a, b)
{
    switch (groupedby)
    {
        case "assignee":
        case "reporter":
        case "verifier":
            {
                var userA = usersHash[a.key];
                var userB = usersHash[b.key];
                if (!userA && !userB)
                {
                    return 0;
                }
                if (userA && !userB)
                {
                    return -1;
                }
                if (!userA && userB)
                {
                    return 1;
                }

                return (userA.name.compare(userB.name));

            }
            break;
        case "milestone":
            {

                var sprintA = sprintHash[a.key];
                var sprintB = sprintHash[b.key];

                if (!sprintA && !sprintB)
                {
                    return 0;
                }
                if (sprintA && !sprintB)
                {
                    return -1;
                }
                if (!sprintA && sprintB)
                {
                    return 1;
                }
                return (sprintA.startdate - sprintB.startdate);

            }
            break;
    }
    return 0;
}

function _itemClickEvent(recid, index, isEdit)
{
    var url = sgCurrentRepoUrl + "/workitem.html?recid=" + recid + "&query=true";
    if (isEdit)
    {
        url += "&edit=true";
    }

    window.location.href = url;

}

function _createItemLink(revno, recid)
{
    var a = $('<a>').text(revno);

    a.attr({ href: sgCurrentRepoUrl + "/workitem.html?recid=" + recid + "&query=true" });
    return a;

}

function menuClicked(e)
{
    e.stopPropagation();
    e.preventDefault();
    var action = $(this).attr("action");
    var recid = $("#itemcontextmenu").data("recid");
    var index = $("#itemcontextmenu").data("index");
    $(".lichangestatus").remove();
    $(".filterContextSubmenu").hide();

    _cancelFormEdits();

    if (action == "view")
    {
        _itemClickEvent(recid, index);
    }
    else if (action == "edit")
    {
        _itemClickEvent(recid, index, true);
    }
    else if (action == "logwork")
    {
        var lwControl = new logWorkControl(recid, {
            afterSave: function (form)
            {
                var rec = form.getSerializedFieldValues();
                var est = rec.estimate;
                var work = [];
                $.each(rec.work, function (i, v) { if (v._delete != "true") work.push(v); });

                _updateWork(work, est, rec.recid);

            }
        });

        lwControl.display();
        $("#itemcontextmenu").hide();
    }

    else if (action == "changestatus")
    {
        var vals = template.rectypes.item.fields["status"].constraints.allowed;

        $.each(vals, function (i, v)
        {
            var curItem = itemsHash[recid];
            if (v != curItem.status)
            {
                var li = $("<li>").text(v).addClass("lichangestatus");
                li.data("recid", recid);
                if (!(sprintHash[curItem.milestone].releasedate && v == "open"))
                {
                    li.click(function (e)
                    {
                        _changeItemValue("status", $(this).text(), $(this).data("recid"), index);

                    $("#divStatusSubmenu").hide();
                    if (allFetched)
                    {
                        _recalculateSummary();
                    }

                    });
                }
                else
                {
                    li.addClass("disabled").attr("title", "Item cannot have open status because it is in a released milestone.");
                }
                $("#ulContextStatus").append(li);
            }

        });

        showContextSubMenu(recid, $("#liChangeStatus"), $("#divStatusSubmenu"));
    }
    else if (action == "postpone")
    {
        if (!milestones)
        {
            vCore.fetchSprints({
                includeLinked: false,
                force: true,
                success: function (data, status, xhr)
                {
                    milestones = data;
                    milestones.push({ name: "Product Backlog", recid: "", startdate: Number.MAX_VALUE });
                    displayMilestoneMenu(milestones, recid, index);
                }
            });
        }
        else
        {
            displayMilestoneMenu(milestones, recid, index);
        }
    }
    else
    {
        throw ("Unknown action: " + action);
    }
}

function displayMilestoneMenu(milestones, recid, index)
{
    $(".lipostpone").remove();

    milestones.sort(function (a, b)
    {
        return (a.startdate - b.startdate);
    });
    $.each(milestones, function (i, v)
    {
        var rid = v.recid;
        var mile = itemsHash[recid].milestone;

        if (!mile)
            mile = "";

        if (rid != mile && !v.releasedate)
        {
            var li = $("<li>").text(v.name).addClass("lipostpone");
            li.data("recid", recid);
            li.data("mileid", v.recid);
            li.click(function (e)
            {
                _changeItemValue("milestone", $(this).data("mileid"), $(this).data("recid"), index);

                $("#divPostponeSubmenu").hide();
                $("#itemcontextmenu").hide();

            });
            $("#ulContextPostpone").append(li);
        }

    });
    showContextSubMenu(recid, $("#liPostpone"), $("#divPostponeSubmenu"));
}

function getRemainingTime(item)
{
    var sum = 0;
    if (item.work)
    {
        sum = _getTimeWorked(item.work);
    }
    var orig = item["estimate"];
    if (!isNaN(orig))
    {
        var remaining = parseInt(orig) - sum;
        if (remaining >= 0)
        {
            return Math.max(remaining, 0);
        }
    }
    return 0;
}

function getLoggedTime(item)
{
    var sum = 0;
    if (item.work)
    {
        sum = _getTimeWorked(item.work);
    }
    return parseInt(sum);
}

function _createTableRow(item, index, key)
{
    var tr = $("<tr>").attr("id", "tr" + item.recid).addClass("tr" + key).addClass("trReader");
    tr.data("recid", item.recid);
    tr.data("itemIndex", index);
    $.each(columns, function (row, col)
    {
        //the original estimate column went away and showing it will give unexpected results
        //when the original estimate field goes away in the template
        //this check an go away.
        if (col != "estimate")
        {
            var text = item[col];

            var td = $("<td>");
            switch (col)
            {
                case "id":
                    {
                        var a = _createItemLink(text, item.recid); // $('<a>').text(text).attr({ "title": "view item", href: sgCurrentRepoUrl + "/workitem.html?recid=" + item.recid });
                        _menuWorkItemHover.addHover(a, item.recid);
                        td.append(a);
                        td.addClass(col);
                    }
                    break;
                //todo use rectype for this somehow
                case "assignee":
                case "reporter":
                case "verifier":
                    {
                        td.text(item[col + "name"] || _getUserFriendlyValue(col, item[col]));
                        td.addClass(col);
                    }
                    break;

                case "logged_time":
                    {
                        td.text(vTimeInterval.format(item["loggedTimeValue"]));
                        td.addClass("logged_time");
                        td.data("origval", item["loggedTimeValue"]);
                    }
                    break;
                case "remaining_time":
                    {
                        var val = item["remainingTimeValue"];
                        if (val > 0)
                        {
                            td.text(vTimeInterval.format(val));
                        }
                        else
                        {
                            var txt = (item["loggedTimeValue"] ? "Complete" : "");
                            td.text(txt);
                        }

                        td.addClass("remaining_time");
                        td.data("origval", val);
                    }
                    break;
                case "last_timestamp":
                case "created_date":
                    {
                        if (item[col])
                        {
                            var d = new Date(parseInt(item[col]));
                            td.text(shortDateTime(d));
                        }
                        td.addClass(col);
                    }
                    break;
                case "milestone":
                    {
                        td.text(item[col + "name"] || _getUserFriendlyValue(col, item[col]));
                        td.addClass("milestone");
                    }
                    break;
                case "stamp":
                    {
                        td.addClass("stamp");
                        if (!item[col])
                            break;

                        for (var i = 0; i < item[col].length; i++)
                        {
                            var name = $("<a>").text(item[col][i].name);
                            name.attr("href", sgCurrentRepoUrl + "/workitems.html?stamp=" + vCore.urlEncode(item[col][i].name) + "&sort=priority&groupedby=status&columns=id%2Ctitle%2Cassignee%2Creporter%2Cpriority%2Cmilestone%2Cstamp&maxrows=100&skip=0");
                            var cont = $("<div>").addClass("cont");
                            var stamp = $("<span>").addClass("stamp").html(name);
                            cont.append(stamp);

                            td.append(cont);
                        }
                    }
                    break;
                default:
                    {
                        td.text(text);
                        td.addClass(col);
                    }
                    break;
            }

            tr.append(td);
        }
    });
    var tdSave = $("<td>").addClass("savecancel");
    var savecancel = $("<div>").css({ "min-width": "30px", "width": "30px", "white-space": "nowrap" });
    tdSave.html(savecancel);
    tr.append(tdSave);
    var tdcontext = $("<td>");
    var spacer = $('<div>').addClass("spacer");
    tdcontext.append(spacer);

    var span = $("<span>").addClass("linkSpan");
    if (!isTouchDevice())
        span.hide();
    var img = $(document.createElement('img')).attr('src', sgStaticLinkRoot + "/img/contextmenu.png");
    var imgEdit = $(document.createElement('img')).attr('src', sgStaticLinkRoot + "/img/edit.png");
    var a = $("<a>").html(img).attr({ "href": "#", "title": "view context menu", "id": "context" + item.recid }).addClass("small_link");
    var aEdit = $("<a>").html(imgEdit).attr({ "href": "#", "title": "edit mode", "id": "edit" + item.recid }).data("recid", item.recid).addClass("small_link");

    span.append(aEdit);
    span.append(a);

    tdcontext.append(span);
    tr.prepend(tdcontext);
    a.click(function (e)
    {
        e.preventDefault();
        e.stopPropagation();

        showCtxMenu($(this).attr("id"), $("#itemcontextmenu"), { "recid": item.recid, "index": index });
    });

    aEdit.click(function (e)
    {
        e.preventDefault();
        e.stopPropagation();

        editRow({ "recid": item.recid, "index": index });
        return false;
    });

    return tr;

}
function hideAddNewItem()
{
    $(".divaddnewitem").hide();
    $("a.newwi").show();
    
    currentItem = null;
    currentAddText = null;
}

function checkDirtyItem()
{
    //if this is an add, check to see if the title has text
    if (!editing)
        return false;

    else if (currentAddText)
    {
        if (currentAddText.val() && currentAddText.val().length)
        {
            return true;
        }
        return false;
    }
    else
    {
        if (!filterForm)
            return false;
        var newVals = filterForm.getFieldValues();
        for (var i in newVals)
        {
            var oldVal = currentItem[i];
            var newVal = newVals[i];
          
            if (newVal != oldVal)
            {
                return true;
            }

        }
        return false;
    }
}

function editRow(editItem, focusControlId)
{
    _cancelFormEdits();
    editing = true;
    hideAddNewItem();
    filterForm.removeSubForm(workController.form);
    var tr = $("#tr" + editItem.recid);
    var trEdit = $("<tr>").addClass("trEdit");
    trEdit.append($("<td>").addClass("spacer hovered"));
    var item = itemsHash[editItem.recid];
    currentItem = item;
    item.rectype = "item";
    filterForm.ready(false);
    filterForm.setRecord(item);

    if ($.inArray("logged_time", columns) >= 0 || $.inArray("remaining_time", columns) >= 0)
    {
        workController.setWitForFilters(item);
        filterForm.addSubForm(workController.form);
    }

    filterForm.ready(true);
    var rowHeight = 24;
    var topPadding = 0;
    //$("#imgButtonSaveEdits").data("recid", item.recid);
    $(".divEditItem input").data("recid", item.recid);
    $(".divEditItem select").data("recid", item.recid);
    $(".divEditItem textarea").data("recid", item.recid);

    $.each(columns, function (i, v)
    {
        var td = tr.find("td." + v);
        var tdEdit = td.clone();
        trEdit.append(tdEdit);

        if (td && td.length)
        {
            var field = filterForm.fieldsHash[v];

            if (field)
            {
                if (field[0].nodeName.toLowerCase() == "textarea")
                {
                    var fontsize = 13; // parseInt(field.css("font-size"));
                    var rows = Math.floor(tr.height() / fontsize);

                    field.attr("rows", rows);

                    if (rows > 1)
                    {
                        rowHeight = rows * fontsize + fontsize;
                    }
                    field.height(rowHeight - 5);
                }

                else
                {
                    topPadding = (rowHeight - 20) / 2;
                }
                field.css({ "width": "99%" });
            }
            var pos = td.offset();
            var div = allcolumns[v].EditDiv;

            if (div && div.length)
            {
                div.css(
                {
                    "padding-top": topPadding,
                    "vertical-align": "middle"

                });
                div.height(tr.height());
                tdEdit.html(div);
                div.show();
            }
        }

    });


	var deb = $('#divEditButtons');
    deb.css("white-space", "nowrap").
		show();
    trEdit.append($("<td>").append(deb).addClass('hovered'));
    tr.after(trEdit);
    tr.hide();
    if (focusControlId)
    {
        $("#" + focusControlId).focus();
    }
    else
    {
        var possibleFocused = ["textarea", "select", "input"];
        var focused = null;
        for (var i = 0; i < columns.length; i++)
        {
            if (allcolumns[columns[i]].EditDiv && allcolumns[columns[i]].EditDiv.length)
            {
                for (var j = 0; j < possibleFocused.length; j++)
                {
                    focused = allcolumns[columns[i]].EditDiv.find(possibleFocused[j]);
                    if (focused && focused.length > 0)
                    {
                        break;
                    }
                }
                break;
            }

        }
        $(focused).focus();
    }

}

function _getTimeWorked(worked)
{
    var sum = 0;
    $.each(worked, function (i, t)
    {
        if (t && t.amount)
        {
            sum += parseInt(t.amount);
        }
    });
    return sum;
}

function sortHashByKeys(inputHash, keySortFunction)
{
    var aTemp = [];
    $.each(inputHash, function (key, val)
    {
        aTemp.push({ "key": key, "value": val });
    });

    aTemp.sort(keySortFunction);

    var aOutput = {};

    $.each(aTemp, function (key, val)
    {
        aOutput[val.key] = val.value;

    });
    return aOutput;
}

function findItemInGroup(itemarray, recid)
{
    for (var i = 0; i < itemarray.length; i++)
    {
        var curItem = itemarray[i].item;
        if (curItem.recid == recid)
        {
            return true;
        }

    }
    return false;
}
function _appendToSummary(item)
{
    if (!item.estimate || item.estimate == 0)
    {
        countNoEst++;
    }
    var nEst = parseInt(item.estimate);

    if (!isNaN(nEst))
    {
        totalEst += parseInt(nEst);
    }

    if (item.work)
    {
        var tw = _getTimeWorked(item.work);
        totalWorked += tw;
    }

}

function _recalculateSummary()
{
    countNoEst = 0;
    totalEst = 0;
    totalWorked = 0;

    $.each(items, function (i, v)
    {
        _appendToSummary(v);
    });
    _setSummary(items.length, countNoEst, totalEst, totalWorked);
}

function _displayItems(grouped, isLocal)
{
    var scrollPos = $(window).scrollTop();

    if (isLocal)
    {
        _setColumns(template);
    }
    var currItemsName = null;
    if (getQueryStringVal("filter") == "default")
    {
        currItemsName = "My Current Items";
    }
    if (getQueryStringVal("filter") == "default" && !$.deparam.fragment()["filter"] && !$.deparam.fragment()["name"] && !criteriaChanged)
    {
        $("#filtername").text(currItemsName);
        vCore.setPageTitle(currItemsName);
    }
    else
    {
        var params = $.deparam.fragment();

        if (params["name"] || criteriaChanged)
        {
            $("#filtername").text((params["name"] || filtername || currItemsName || "") + "*");
        }
        else
        {
            $("#filtername").text(filtername || "");
        }
    }

    if (newQuery)
    {
        $("#filterresults").find("table.datatable").remove();
        groupedHash = {};
        countNoEst = 0;
        totalEst = 0;
        totalWorked = 0;
    }

    var table = $("#filterresults").find("table.datatable");
    if (!table || !table.length)
    {
        table = _addNewTable();
    }
    if (grouped)
    {
        groupedby = grouped;
        filter.groupedby = grouped;
    }

    //see if there is a new item link at the bottom, if so remove it
    var lastRow = $("#filterresults tr").last();
   
    var key = groupedby.toLowerCase();

    var subItems = items.slice(startDisplayAt + numAdded, items.length);

    var groupCount = 0;
    var lastKey = null;
    var lastType = null;
    var tablekey = null;
    $.each(subItems, function (i, v)
    {
        itemsHash[v.recid] = v;
        _appendToSummary(v);
        v.remainingTimeValue = getRemainingTime(v);
        v.loggedTimeValue = getLoggedTime(v);

        tablekey = v[key];
        if (!tablekey || tablekey == "undefined" || tablekey == "0")
        {
            tablekey = "none";
        }
        if (!groupedHash[tablekey])
        {
            _addTableGroup(table, groupedby, tablekey, lastType, lastKey);
            groupCount++;
        }
        else
        {
            if (lastRow && lastRow.hasClass("tr-newwi"))
            {
                lastRow.remove();
            }
        }
        lastKey = tablekey;
        lastType = groupedby;
        groupedHash[tablekey] = true;

        var tr = _createTableRow(v, startDisplayAt + i + numAdded, tablekey);

        table.append(tr);


    });

    $("#itemcontextmenu li").unbind("click");
    $("#itemcontextmenu li").click(menuClicked);

    if (items.length == 0 && groupedby == "none")
    {
        _addTableGroup(table, groupedby, "none");
    }

    //the last group wouldn't have
    //gone into the code that adds a footer so add it if
    //needed
    _displayTableFooter(table, groupedby, tablekey, lastType, lastKey);

    bindObservers();
    _setDescription(filter.criteria, $("#filterdesctext"));
    if (allFetched)
    {
        _setSummary(items.length, countNoEst, totalEst, totalWorked);
    }
    $('#wiLoading').hide();

    $("#groupprogress").hide();
    gettingItems = false;
    enableLink($("#link_criteria"), _criteriaClick);
    enableLink($("#link_groupby"), _groupbyClick);
    enableLink($("#link_columns"), _columnsClick);
    enableLink($("#link_savefilter"), _saveClick);

    saveURLToSession();

    utils_doneSaving();
    $(window).scrollTop(scrollPos);
}

function _setSummary(total, noEstimate, totalEst, totalWorked)
{
    var remaining = (totalEst - totalWorked);
    var complete = totalWorked;
    $("#tdSummaryTotal").text(total);
    $("#tdNoEstimate").text(noEstimate);
    $("#tdWorkRemaining").text(vTimeInterval.format(remaining));
    $("#tdWorkComplete").text(vTimeInterval.format(complete));

    $("#filtersummary").show();
}

function enableLink(link, clickevent)
{
    link.unbind("click");
    link.removeClass("disabled").attr("href", "#");
    link.click(function (e)
    {
        e.preventDefault();
        e.stopPropagation();
        _cancelFormEdits();
        clickevent(e);
    });
}

function disableLink(link)
{
    link.addClass("disabled").removeAttr("href");
    link.unbind("click");
}


function _deleteFilterClick(e)
{
    e.preventDefault();
    e.stopPropagation();
    // deleteFilter(filter.recid, filter.name);
    confirmDialog("Delete filter " + filter.name + "?", $("#link_criteria"), deleteFilter, filter.recid, null, null, [0, -100]);

}

function _getItems(changeEvent)
{
    $(".trEdit").hide();
    $(".trReader").show();

    disableLink($("#link_criteria"));
    disableLink($("#link_groupby"));
    disableLink($("#link_columns"));
    disableLink($("#link_savefilter"));
    disableLink($("#link_deletefilter"));

    $("#filtersummary").hide();

    var opts = $.param.fragment();
    var qs = $.param.querystring();

    if (opts || qs || newFilter)
    {
        if (items && items.length)
        {
            $('#divmainprogress').text("Loading work items...");
            $('#wiLoading').show();
        }
        utils_savingNow("Loading work items...");
        scrollPosition = $('#wiLoading').offset().top;
        $('#newfilter').hide();
        var params = {};

        gettingItems = true;

        var url = sgCurrentRepoUrl + "/filteredworkitems.json";

        if (!opts)
        {
            params = $.deparam.querystring();
        }
        else
        {
            var tempQS = $.deparam.querystring();
            var tempOpts = $.deparam.fragment();
            $.each(tempQS, function (i, v)
            {
                if (i != "filter")
                {
                    params[i] = v;
                }
            });
            //let hash params override qs params
            $.each(tempOpts, function (i, v)
            {
                params[i] = v;
            });
        }

        params["maxrows"] = numResults;
        params["skip"] = numFetched;

        delete params.name;

        //  url = $.param.querystring(url, params);

        newFilter = false;
        template = null;

        //this is a POST due to query string limitations
        //really it is just getting data so don't show the unload warning
        $(window).unbind('beforeunload');

        vCore.ajax(
	        {
	            type: "POST",
	            url: url,
	            abortOK: true,
	            data: JSON.stringify(params),
	            success: function (data, status, xhr)
	            {
	                startDisplayAt = numFetched;
	                $.each(data.results, function (i, v)
	                {
	                    if (!itemsHash[v.recid])
	                    {
	                        items.push(v);
	                    }
	                });

	                if (!data.results || data.results.length != numResults || data.results.length == 0)
	                {
	                    allFetched = true;
	                }
	                numFetched += data.results.length;
	                filter = data.filter;

	                //don't update the filter on the page
	                //if it hasn't been saved.
	                if (data)
	                {
	                    template = data.template;
	                    if (filter && filter.name)
	                    {
	                        filtername = filter.name;
	                        _saveVisitedFilter(filter.name, filter.recid);
	                        vCore.setPageTitle(filter.name);
	                        enableLink($("#link_deletefilter"), _deleteFilterClick);
	                    }
	                    else
	                    {
	                        disableLink($("#link_deletefilter"));
	                    }
	                    if (data.users && data.users.length > 0)
	                    {
	                        $.each(data.users, function (i, user)
	                        {
	                            usersHash[user.recid] = user;
	                        });
	                        usersHash["currentuser"] = { name: "Current User", email: "Current User" };
	                        usersHash["(none)"] = { name: "(unassigned)", email: "(unassigned)" };
	                    }
	                    if (data.sprints && data.sprints.length > 0)
	                    {
	                        $.each(data.sprints, function (s, sprint)
	                        {
	                            sprintHash[sprint.recid] = sprint;
	                        });
	                        sprintHash["(none)"] = { name: "Product Backlog", startdate: Number.MAX_VALUE };
	                        sprintHash["currentsprint"] = { name: "Current Milestone" };
	                    }
	                    if (data.filter.columns)
	                    {
	                        columns = data.filter.columns.split(",");
	                    }
	                    if (data.filter.groupedby)
	                    {
	                        groupedby = data.filter.groupedby;
	                    }
	                    if (data.filter.sort)
	                    {
	                        sort = data.filter.sort;
	                        var split = sort.split(" ");
	                        if (split.length > 1)
	                        {
	                            sortHash[split[0]] = split[1];
	                        }
	                        else
	                            sortHash[split[0]] = "#ASC";


	                    }
	                }
	            },
	            complete: function ()
	            {
	               // $(window).bind('beforeunload', vCore.catchBeforeUnload);
	                _getAllColumnValues(template, null, true);
					$(window).resize();
	            },
	            error: function (xhr, textStatus, errorThrown)
	            {
	                if (xhr.statusText.toLowerCase() == "not found")
	                {
	                    _deleteFromFiltersMenu(getQueryStringVal("filter"));
	                }
	                $('#wiLoading').hide();
	            }

	        });

    }
}


function _updateItemFromRecord(form, record)
{
    var oldItem = null;
    var index = 0;
    for (var i = 0; i < items.length; i++)
    {
        if (items[i].recid == record.recid)
        {
            oldItem = items[i];
            index = i;
            break;
        }
    }
    var fields = [];
    for (var r in record)
    {
        if (oldItem[r] != record[r])
        {
            if (r != "work")
            {
                fields.push({ field: r, value: record[r] });
            }
        }
    }

    var updateItem = { recid: record.recid, values: fields, index: index };

    _updateItemValue(updateItem);

}

function _updateWork(work, estimate, recid)
{
    for (var i = 0; i < items.length; i++)
    {
        if (items[i].recid == recid)
        {
            var item = items[i];
            var tr = $("#tr" + recid);
            item.work = work;
            item.estimate = estimate;
            var rt = getRemainingTime(item);
            var lt = getLoggedTime(item);

            item["remainingTimeValue"] = rt;
            item["loggedTimeValue"] = lt;

            var td = tr.find("td.logged_time");

            td.text(vTimeInterval.format(item["loggedTimeValue"]));
            td.data("origval", item["loggedTimeValue"]);

            td = tr.find("td.remaining_time");
            var val = item["remainingTimeValue"];
            if (val > 0)
            {
                td.text(vTimeInterval.format(val));
            }
            else
            {
                var txt = (item["loggedTimeValue"] ? "Complete" : "");
                td.text(txt);
            }
            td.data("origval", val);
        }
    }
    if (allFetched)
    {
        _recalculateSummary();
    }

}

function _itemMatchesFilter(item)
{
    var match = true;

    if (filter && filter.criteria)
    {
        $.each(filter.criteria, function (i, v)
        {
            var tmp = v;
            if (i != "stamp")
            {
                var value = item[i];

                if (i == "estimated" && !value)
                {
                    value = "ignore";
                }
                else if (!value)
                {
                    value = "none";
                }

                if (tmp.indexOf("currentuser") >= 0)
                {
                    tmp += "," + sgUserName;
                }
                if (tmp.indexOf("currentsprint") >= 0)
                {
                    tmp += "," + sgCurrentSprint;
                }
                if (tmp.indexOf(value) < 0)
                {
                    match = false;
                }
            }
        });
    }
    return match;

}

function findLastIndex(field, value)
{
    var lastIndex = -1;
    if (value == "none")
    {
        value = "";
    }
    for (var i = 0; i < items.length; i++)
    {
        if (items[i][field] == value)
        {
            lastIndex = i;
        }
    }
    return lastIndex;
}

function findFirstIndex(field, value)
{
    for (var i = 0; i < items.length; i++)
    {
        if (items[i][field] == value)
        {
            return i;
        }
    }
    return -1;
}


function _findSpot(field, value)
{
    var lastItemRetrieved = items[items.length - 1];

    var values = [];
    //for milestones and users we need to get the full list since
    //the dropdowns only show active miles and users
    if (field == "milestone")
    {
        var miles = filterForm.sprints;

        miles.sort(function (a, b)
        {
            return a.startdate - b.startdate;
        });
        values = $.map(miles, function (elt, i) { return elt.recid; });
    }
    else if (field == "assignee" || field == "reporter" || field == "verifier")
    {
        var usrs = filterForm.users;
        usrs.sort(function (a, b)
        {
            return (a.name.compare(b.name));
        });
        values = $.map(usrs, function (elt, i) { return elt.recid; });
    }
    else
    {
        domelts = $("#item_" + field + " option");

        values = $.map(domelts, function (elt, i) { return $(elt).val(); });
    }

    var index = $.inArray(value, values);
    var lastIndex = $.inArray(lastItemRetrieved[field], values);

    if (index < lastIndex)
    {
        for (var i = index; i < values.length; i++)
        {
            var toIndex = findFirstIndex(field, values[i]);
            if (toIndex >= 0)
            {
                return toIndex;
            }
        }
        return 0;
    }
    return -1;
}
function _updateItemValue(updateitem)
{
    editing = false;
    var tr = $("#tr" + updateitem.recid);
    var itemIndex = tr.data("itemIndex");

    var item = updateCurrentItem(updateitem.recid, updateitem.values);
    if (!_itemMatchesFilter(item))
    {
        items.splice(updateitem.index, 1);
        _resetDisplay();
        _displayItems(null, true);
        return true;
    }
    for (var i = 0; i < updateitem.values.length; i++)
    {
        var tdStatus = tr.find("td." + updateitem.values[i].field);

        if (filter.groupedby == updateitem.values[i].field)
        {
            items.splice(updateitem.index, 1);

            var toIndex = findLastIndex(updateitem.values[i].field, updateitem.values[i].value);
            if (toIndex >= 0)
            {
                items.splice(toIndex + 1, 0, item);
            }

            else
            {
                var spot = _findSpot(updateitem.values[i].field, updateitem.values[i].value);
                if (spot >= 0)
                {
                    items.splice(spot, 0, item);
                }
                else if (allFetched)
                {
                    items.push(item);
                }
            }
            _resetDisplay();
            _displayItems(null, true);
            return;
        }
        else if (tdStatus && tdStatus.length)
        {
            if (updateitem.values[i].field == "milestone" || updateitem.values[i].field == "assignee" || updateitem.values[i].field == "verifier")
            {
                tdStatus.text(item[updateitem.values[i].field + "name"]);
            }
            else
            {
                tdStatus.text(item[updateitem.values[i].field]);
            }

        }
    }
    utils_doneSaving();
    if (allFetched)
    {
        _recalculateSummary();
    }
}

function _changeItemValue(fld, val, id, index)
{
    utils_savingNow("Saving work item...");
    var jsonVal = {};
    jsonVal[fld] = val;
    var vals = [{ field: fld, value: val}];
    var item =
    {
        recid: id,
        json: jsonVal,
        values: vals,
        afterSuccess: _updateItemValue,
        url: sgCurrentRepoUrl + "/workitem-full/" + id + ".json",
        index: index
    };

    updateQueue.push(item);
    _deQueueWorkItemUpdate();
}


function _processAddFilter(data)
{
    var n = textBoxFilterName.val().trim();

    if ($.inArray(n, currentFilters) >= 0)
    {
        reportError("The " + n + " filter already exists. Filter names must be unique.");
        return;
    }
    filter.name = n;

    addNewFilter(filter);
}

function submitFilterName()
{
    var n = textBoxFilterName.val().trim();

    if (n && n.length > 0)
    {
        var curName = filter.name || filtername;
        var newName = n != curName;

        var curRecid = newRecid || $.deparam.fragment()["filter"];

        var recid = curRecid || getQueryStringVal("filter");

        if (recid && recid != "default" && !newName)
        {
            updateFilter(recid, filter);
            filter.name = filtername;
            vCore.setPageTitle(filter.name);
        }
        else
        {
            if (!currentFilters)
            {
                vCore.fetchFilters(
                {
                    success: function (data)
                    {
                        if (data)
                        {
                            currentFilters = [];
                            $.each(data, function (i, v)
                            {
                                currentFilters.push(v.name);
                            });
                        }
                        if ($.browser.msie)
                            sleep(500);
                    },
                    complete: _processAddFilter
                });
            }
            else
            {
                _processAddFilter();
            }
        }

    }
    else
    {
        reportError("name field is required");
    }
}

function _setUpSaveForm()
{
    saveForm = $("<div>").attr("id", "divsavefilter");

    textBoxFilterName = $("<input>");
    textBoxFilterName.attr({ "type": "text", id: "textboxfiltername" });

    saveForm.append(textBoxFilterName);
    textBoxFilterName.focus(function ()
    {
        $(this).select();
    });
    textBoxFilterName.keydown(function (e1)
    {
        if (e1.which == 13)
        {
            e1.preventDefault();
            e1.stopPropagation();

            submitFilterName();
            saveForm.vvdialog("vvclose");
        }
    });
}

function _criteriaClick(e)
{
    e.preventDefault();
    e.stopPropagation();

    _showFilterForm(filter);
    //filterControl.draw("filtercontrol", $filterdialog);
};

function _saveClick(e)
{
    e.preventDefault();
    e.stopPropagation();

    var params = $.deparam.fragment();
    filter.recid = getQueryStringVal("filter");
    if (params && params.name && filter.recid != "default")
    {
        filtername = params.name;
    }

    textBoxFilterName.val(filtername);

    saveForm.vvdialog({
        title: "Save Filter As",
//        position: [$("#link_savefilter").offset().left, $("#link_savefilter").offset().top + 20],

        resizable: false,
        buttons:
		{
		    "OK": function ()
		    {
		        submitFilterName();
		        $(this).vvdialog("vvclose");
		    },
		    "Cancel": function ()
		    {
		        $(this).vvdialog("vvclose");
		    }
		},
        close: function (event, ui)
        {
            $("#backgroundPopup").hide();
        }
    });
}

function _columnsClick(e)
{
    e.preventDefault();
    e.stopPropagation();

    _showColumnsDialog();
}
function _groupbyClick(e)
{
    e.preventDefault();
    e.stopPropagation();
    _showGroupedByDialog();

    $('.ui-dialog-buttonpane > button:last').focus();
}

function hashStateChanged(e)
{
    if (!gettingItems && !noRefresh)
    {
        if (localRefresh)
        {
            _resetDisplay();
            if (!newFilter)
            {
                _displayItems(groupedby, true);
            }
            localRefresh = false;
        }
        else
        {
            _resetResults();
            _getItems(true);
        }
    }
    noRefresh = false;
}

function checkIfCriteriaChanged(newCriteria)
{
    if ((filter && filter.criteria) && !newCriteria)
    {
        return true;
    }
    else if ((!filter || !filter.criteria) && newCriteria)
    {
        return true;
    }
    else
    {
        for (var i in filter.criteria)
        {
            if (newCriteria[i] != filter.criteria[i])
                return true;
        }

        for (i in newCriteria)
        {
            if (filter.criteria[i] != newCriteria[i])
                return true;
        }
    }
    return false;
}

function filterCriteriaStateChanged()
{
    var fragments = $.deparam.fragment();
    if (fragments["reporter"] || fragments["assignee"] || fragments["milestone"] || fragments["priority"]
    || fragments["status"] || fragments["verifier"] || fragments["stamp"])
    {

        return true;
    }
    return false;
}


function sgKickoff()
{   
    filterForm = new vForm(
        {
            rectype: "item",
            errorFormatter: bubbleErrorFormatter,
            method: "PUT",
            handleUnload: true,
            dirtyHandler: function () { return checkDirtyItem(); },
            queuedSave: false,
            duringSubmit: {
                start: function ()
                {
                    utils_savingNow("Saving work item...");
                },

                finish: function ()
                {
                    utils_doneSaving();
                    $(".trEdit").hide();
                    $(".trReader").show();
                }
            }
        });

 
    workController = new workFormController();

    workController.createWorkForm(true, true);

    workController.form.options.duringSubmit = {
        start: function ()
        {
            utils_savingNow("Saving work item...");
        },

        finish: function ()
        {
            utils_doneSaving();
        }
    };

    $("#filtersummary").hide();
    _setUpSaveForm();

    if (!$.param.querystring() && !$.param.fragment())
    {
        newFilter = true;
        $("#filtername").text("Create New Filter");
        // _getAllColumnValues();
        enableLink($("#link_criteria"), _criteriaClick);
        enableLink($("#link_groupby"), _groupbyClick);
        enableLink($("#link_columns"), _columnsClick);
        $('#newfilter').html("Click the <b>criteria</b> link to choose critera to add to the new filter").show();
    }
    else
    {
        var fn = $.deparam.querystring()["filter"];
        if (fn == "default" && !sgCurrentSprint)
        {
            var a = $('<a></a>').
				text('milestones').
				attr('href', sgCurrentRepoUrl + "/milestones.html");
            var emsg = $("<span class='error'>*There is no current milestone configured. Go to the </span>").
				append(a).
				append(" page to set a current milestone.").
				appendTo($('#filterresults'));

        }
        _getItems();
        $('#newfilter').hide();
        newRecid = $.deparam.fragment()["filter"];
    }

    $("#filterpagecontent").show();

    $filterdialog = $('<div>')
        .css({"padding": "0px",  "overflow-y": "scroll" })    
		.vvdialog({
		    // position: [null, $("#link_criteria").offset().top + 20],
		    resizable: true,
		    width: "95%",
		    height: "700",
		    dialogClass: "divFilterDialog",
		 
		    buttons:
            {
                "Submit": function ()
                {
                    $('#filterresults').find("span.error").remove();
                    state = filterControl.getSelected();

                    criteriaChanged = checkIfCriteriaChanged(state);
                    if (!criteriaChanged)
                    {
                        $(this).vvdialog("vvclose");
                        return;
                    }
                    filter.criteria = state;

                    disableLink($('#link_deletefilter'));
                    //clear the state
                    $.bbq.pushState({}, 2);

                    //add the non-criteria items to
                    //hash to preserve these values even
                    //if no criteria is selected
                    state["sort"] = sort;
                    state["groupedby"] = groupedby;
                    state["columns"] = columns.join(",");

                    var params = $.deparam.fragment();
                    var defaultFilterName = "";
                    if (getQueryStringVal("filter") == "default")
                        defaultFilterName = "My Current Items";

                    state["name"] = filter.name || filtername || defaultFilterName;
                    if (JSON.stringify(state) == "{}")
                    {
                        $(window).trigger('hashchange');
                    }
                    else
                    {
                        $.bbq.pushState(state);
                    }

                    $(this).vvdialog("vvclose");

                },
                "Cancel": function ()
                {
                    $(this).vvdialog("vvclose");
                }
            },
		    close: function (event, ui)
		    {
		        $("#backgroundPopup").hide();
		    },
		    autoOpen: false,
		    title: 'Choose Filter Criteria'
		});

    $("#textEstimate").keydown(function (e1)
    {
        if (e1.which == 13)
        {
            e1.preventDefault();
            e1.stopPropagation();
            _updateTimeEstimate();
        }
        if (e1.which == 27)
        {
            $("#edittime").hide();
        }
    });

    $("#textLoggedTime").keydown(function (e1)
    {
        if (e1.which == 13)
        {
            e1.preventDefault();
            e1.stopPropagation();
            _updateTimeLoggged();
        }
        if (e1.which == 27)
        {
            $("#edittimelogged").hide();

        }
    });

    if (newFilter)
    {
        _showFilterForm();
    }
    $(window).bind('hashchange', hashStateChanged);



}
