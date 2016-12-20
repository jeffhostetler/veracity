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

var allFilterNames = [];
var filters ={};

function deleteFilter(recid)
{
    _savingNow("Deleting work item filter...");
    $('#filterprogress').show();
    vCore.ajax(
	{
	    type: "DELETE",
	    url: sgCurrentRepoUrl + "/filter/" + recid + ".json",
	    success: function (data, status, xhr)
	    {
	        //filter = data;
	        _deleteFromFiltersMenu(recid);
	    
	        getFilters();

	    },
	    error: function (xhr, textStatus, errorThrown)
	    {
	        $('#filterprogress').hide();
	    }
	});

}

function renameFilter(inputID, filterID, linkID, spanID)
{
    var name = $("#" + inputID).val();
    var link = $("#" + linkID);
    var span = $("#" + spanID);
    var filter = filters[filterID];

    var curIndex = $.inArray(name, allFilterNames);

    _savingNow("Saving work item filter...");
    if (curIndex >= 0)
    {      
        if (link.text() == name)
        {
            span.hide();
            link.show(); 
            return;
        }
        reportError("filter name already exists");
        return;

    }
    if (jQuery.trim(name) != "")
    {
        
        $('#filterprogress').show();
        filter.name = name;
        var url = sgCurrentRepoUrl + "/filter/" + filter.recid + ".json";
        vCore.ajax(
	    {
	        type: "PUT",
	        url: url,
	        data: JSON.stringify(filter),
	        success: function (data, status, xhr)
	        {
	            $('#filterprogress').hide();
	            _saveVisitedFilter(name, filter.recid);
	            _setFilterMenu();
	            getFilters();           
	           
	        },
	        error: function (xhr, textStatus, errorThrown)
	        {
	            $('#filterprogress').hide();
	            
	        }
	    });
    }

}

function bindObservers()
{
    enableRowHovers();   
}

function menuClicked(e)
{
    e.stopPropagation();
    e.preventDefault();
    var action = $(this).attr("action");
    var recid = $("#editMenu").data("recid");
    var input = $("#editMenu").data("inputid");
    var link = $("#editMenu").data("linkid");
    var span = $("#editMenu").data("spanid");

    if (action == "rename")
    {
        $("#" + link).hide();
        $("#" + span).show();
        $("#" + input).focus();
        $("#" + input).keydown(function (e1)
        {
            if (e1.which == 13)
            {
                e1.preventDefault();
                e1.stopPropagation();

                renameFilter(input, recid, link, span);

            }
            if (e1.which == 27)
            {
                $("#" + span).hide();
                $("#" + link).show();
            }
        });
        $("#delConfMenu").hide();
        $("#editMenu").hide();
    }
    else if (action == "confirm_delete")
    {
        showDeleteSubMenu(recid);
    }
    else if (action == "delete")
    {
        deleteFilter(recid);
        $("#delConfMenu").hide();
        $("#editMenu").hide();   
    }
    else
        throw ("Unknown action: " + action);

}

function displayFilters(data)
{
    $("#tablefilters td").remove();
    $("#editMenu li").unbind("click");
    $("#delConfMenu li").unbind("click");
    allFilterNames = [];
    filters = {};

    //alert(data.recid);

    data.sort(function (a, b)
    {
        return (a.name.compare(b.name));
    });

    $.each(data, function (i, v)
    {
        filters[v.recid] = v;
        allFilterNames.push(v.name);

        var tr = $("<tr class='trReader'></tr>");
        var tdcontext = $("<td>");
        var spacer = $('<div>').addClass("spacer");
        tdcontext.append(spacer);
        var span = $("<span>").addClass("linkSpan");
        if (!isTouchDevice())
            span.hide();
        var img = $(document.createElement('img')).attr('src', sgStaticLinkRoot + "/img/contextmenu.png").addClass("ctxMenuItemImg");
        var a = $("<a>").html(img).attr({ "href": "#", "title": "view context menu", "id": "context" + v.recid });
        a.click(function (e)
        {
            e.preventDefault();
            e.stopPropagation();
            showCtxMenu("context" + v.recid, null, { "inputid": "text" + v.recid, "recid": v.recid, "linkid": "link" + v.recid, "spanid": "span" + v.recid });
        });
        span.append(a);
        tdcontext.append(span);
        tr.append(tdcontext);

        var tdName = $("<td>").addClass('managefilter-name');
        var tdDesc = $("<td>").addClass('managefilter-desc');
        sort = v.sort;
        groupedby = v.groupedby;
        var d = _setDescription(v.criteria);
        tdDesc.html(d);
        var span = $("<span>").hide();
        span.attr("id", "span" + v.recid);
        var inputName = $("<input>").attr("type", "text").val(v.name);
        inputName.attr("id", "text" + v.recid);
        span.append(inputName);
        var aName = $("<a>").text(v.name).attr({ "id": "link" + v.recid, href: sgCurrentRepoUrl + "/workitems.html?filter=" + v.recid, "title": "display filter results" });

        var submit = $("<input>").attr({ "type": "image", "value": "submit", "src": sgStaticLinkRoot + "/img/save16.png" });

        submit.click(function (e)
        {
            e.preventDefault();
            e.stopPropagation();
            renameFilter("text" + v.recid, v.recid, "link" + v.recid, "span" + v.recid);

        });
        var cancel = $("<input>").attr({ "type": "image", "value": "cancel", "src": sgStaticLinkRoot + "/img/cancel18.png" });
        cancel.click(function (e)
        {
            e.preventDefault();
            e.stopPropagation();
            span.hide();
            aName.show();
        });

        span.append(inputName);
        span.append(submit);
        span.append(cancel);
        tdName.append(aName);
        tdName.append(span);

        tr.append(tdName);
        tr.append(tdDesc);

        $("#tablefilters").append(tr);
    });

    _doneSaving();
    bindObservers();
    $("#editMenu li").click(menuClicked);
    $("#delConfMenu li").click(menuClicked);

}

function getFilters()
{
    if (! sgCurrentRepoUrl)
	return;

    $('#filterprogress').show();

    var url = sgCurrentRepoUrl + "/filters.json?details";
    if (!wimd)
    {
        _savingNow("Loading work item filters...");
    }
    vCore.ajax(
            {
                url: url,
                dataType: 'json',
                success: function (data)
                {
                    displayFilters(data);
                },
                error: function ()
                {
                    $('#filterprogress').hide();
                }

            });
}

function getFilterVals()
{
    _savingNow("Loading work item filters...");
    vCore.ajax({
        url: sgCurrentRepoUrl + "/filtervalues.json",
        dataType: 'json',
        success: function (data, status, xhr)
        {
            for (var f in data.template.rectypes.item.fields)
            {
                var field = data.template.rectypes.item.fields[f];
                var label = field.form ? field.form.label : null;
                if (label)
                {
                    allcolumns[f] = new itemTableColumn(label);
                    if (f == "remaining_time")
                    {
                        label = "Remaining Time";
                        allcolumns["remaining_time"] = new itemTableColumn(label);
                    }
                    else if (f == "estimate")
                    {
                        label = "Estimate";
                        allcolumns["estimate"] = new itemTableColumn(label);
                    }
                }
            }
            allcolumns["stamp"] = new itemTableColumn("Stamps");
            allcolumns["milestone"] = new itemTableColumn("Milestone");
            allcolumns["last_timestamp"] = new itemTableColumn("Last Modified");
            if (data.users && data.users.length > 0)
            {
                $.each(data.users, function (i, user)
                {
                    usersHash[user.recid] = user;
                });
                usersHash["currentuser"] = { email: "Current User", name: "Current User" };
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
            getFilters();
        }

    });

}
function sgKickoff()
{
    getFilterVals();
    $("#link_refreshMenu").click(function (e)
    {
        e.preventDefault();
        e.stopPropagation();
        _setFilterMenu(true);
        getFilters();

    });
      
}
