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

var usersHash = {};
var sprintHash = {};
var allcolumns = {};
var sort = "priority";
var groupedby = "status";

var timevals =
{
    0: "today",
    1: "since yesterday",
    7: "in the last week",
    30: "in the last 30 days",
    90: "in the last 90 days",
    180: "in the last 180 days",
    365: "in the last year"
};

function itemTableColumn(displayName, editDiv)
{
    this.Name = displayName;
    this.EditDiv = editDiv;
}

var wimd = null;

function _savingNow(msg)
{
    wimd = new modalPopup('none', true, { position: "fixed", css_class: "savingpopup" });
    var outer = $(document.createElement('div')).
		addClass('spinning');
    var saving = $(document.createElement('p')).
		    addClass('saving').
		    text(msg).
		    appendTo(outer);
    var p = wimd.create("", outer, null).addClass('info');
    $('#maincontent').prepend(p);

    wimd.show();
}

function _doneSaving()
{
    if (wimd)
        wimd.close();

    wimd = false;
}

//over ride the "modal" option and just do our own
//background modalization since ui.dialog modal and ie have problems
$.widget("custom.vvdialog", $.ui.dialog,
{
    vvopen: function ()
    {
        $("#backgroundPopup").css({
            "opacity": "0.7"
        });
        $("#backgroundPopup").show();

        this.open();

    },
    vvclose: function ()
    {
        $("#backgroundPopup").hide();

        this.close();
    }

});

function _setDescription(params, container)
{
    var cont = container || $("<div>");
    var desc = "All work items ";
 
    var s = "";
    if (params)
    {
        var moreVals = false;
        desc = "All work items ";
        if (params["blocking"])
            desc += "<b>Blocking Items </b>";
        if (params["depending"])
            desc += "<b>Depending on Items </b>";

        $.each(params, function (i, v)
        {
            if (!(i == "estimated" && v == "ignore"))
            {
                if (s.length > 0)
                    s += ", ";
                var val = v;
                if (i == "blocking")
                    blocking = true;
                if (i == "depending")
                    depending = true;
                if (i == "assignee" || i == "verifier" || i == "reporter")
                {
                    var splits = val.split(",");
                    var users = [];

                    $.each(splits, function (u, n)
                    {
                        if (usersHash[n])
                        {
                            users.push(usersHash[n].name);
                        }

                    });
                    val = users.join(",");
                }
                else if (i == "milestone")
                {
                    var splits = val.split(",");
                    var sprints = [];
                    $.each(splits, function (u, n)
                    {
                        if (sprintHash[n])
                        {
                            sprints.push(sprintHash[n].name);
                        }
                    });
                    val = sprints.join(",");
                }
                else if (i == "last_timestamp" || i == "created_date")
                {
                    val = timevals[val];
                }
                if (val)
                {                 
                    var colName = "";
                    if (allcolumns[i])
                    {
                        colName = allcolumns[i].Name;
                    }
                    if (i == "estimated")
                        colName = "Estimated";
                    else if (i == "keyword")
                        colName = "Text";

                    if (colName)
                    {
                        moreVals = true;
                        s += "<b>" + colName + "</b>" + ": " + vCore.htmlEntityEncode(val.split(",").join(", "));
                    }
                }
            }

        });
       


    }

    var sortStr = sort;
    var splits = sortStr.split(" ");
    if (allcolumns[splits[0]])
    {
        sortStr = sortStr.replace(splits[0], allcolumns[splits[0]].Name);
        s += " <b>Sorted by</b> " + sortStr.replace("#DESC", "(descending)");
    }
    
    if (allcolumns[groupedby] && allcolumns[groupedby].Name && allcolumns[groupedby].Name != "none")
    {
        s += " <b>Grouped by</b> " + allcolumns[groupedby].Name;
    }
    cont.html(desc + (moreVals ? " with " : " ") + s);

    return cont;
}
