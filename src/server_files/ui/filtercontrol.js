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


//state: 0 is unchecked, 1 is checked, 2 is intermediate
//options { "onCheck": oncheckfunction, "onUncheck": onuncheckfunction, "nodes": nodes }
function threeStateSelector(state, node, options)
{
    this.state = state || 0;
    this.onCheck = null;
    this.onUncheck = null;
    this.node = node;
     
    if (options)
    {
        if (options.onCheck)
        {
            this.onCheck = options.onCheck;
        }
        if (options.onUncheck)
        {
            this.onUncheck = options.onUncheck;
        }           
    }
   
    var self = this;
   
    this.img = $("<img>").addClass("tristatecheckbox").attr("id", "triImg" + node.value);

    this.drawCheckbox(this.state);
    this.img.click(function (e)
    {
        e.stopPropagation();
        e.preventDefault();
        self.imgClickEvent($(this));       

    });   
  
}

extend(threeStateSelector,
{
    setState: function (val)
    {
        this.drawCheckbox(val);
    },

    imgClickEvent: function (img)
    {
        var self = this;
        var state = img.attr("action");
        if (state == 0 || state == 2)
        {
            self.drawCheckbox(1);
            if (self.onCheck)
                self.onCheck();
        }
        else
        {
            self.drawCheckbox(0);
            if (self.onUncheck)
                self.onUncheck();
        }
    },
    drawCheckbox: function (state)
    {
        var checked = sgStaticLinkRoot + "/img/tristate_check3.png";
        var unchecked = sgStaticLinkRoot + "/img/tristate_check1.png";
        var mix = sgStaticLinkRoot + "/img/tristate_check2.png";

        var self = this;
        switch (state)
        {
            case 0:
                {
                    self.img.attr({ "src": unchecked, "title": "select all", "action": 0 });
                    self.img.removeAttr("checked");
                    self.img.data("state", 0);
                }
                break;
            case 1:
                {
                    self.img.attr({ "src": checked, "title": "select none", "checked": true, "action": 1 });
                    self.img.data("state", 1);
                }
                break;
            case 2:
                {
                    self.img.attr({ "src": mix, "title": "select all", "checked": true, "action": 2 });
                    self.img.data("state", 2);
                }
                break;
        }

    }

});



function checkboxTreeNode(name, value, selectable, nodes, click, isdefault, isinactive)
{
    this.name = name;
    this.value = value;
    this.nodes = nodes || [];
    this.click = click || null;
    this.parent = {};
    this.selectable = selectable || false;
    this.checkbox;
    this.selected = false;
    this.desc_prefix = "";
    this.realname = null;
    this.realvalue = null;
    this.listItem = null;
    this.isdefault = isdefault || false;
    this.isinactive = isinactive || false;

    this.addNode = function (child)
    {
        child.parent = this;
        this.nodes.push(child);
    };

}

//checkedvalues format { fieldname: "value1,value2,value3", fieldname2: value,...}
function checkboxTree(container, checkedvalues, mode, options)
{
    this.container = container || $("#maincontent");
    this.options = options || {};
    this.checkedValues = checkedvalues || {};
    this.mode = mode; //temporary hack    
	this.stampsController = null
    this._initControl();
}


extend(checkboxTree, {
    getFunction: null,
    onsubmit: null,
    title: null,
    nodes: [],
    users: {},
    description: null,
    tristates: {},

    _initControl: function ()
    {
        var self = this;

        var s = $.extend(true, {}, checkboxTree.fetchSettings, self.options);

        if (s.getFunction)
        {
            if (this.mode == "columns")
            {
                this.columnNodes = s.getFunction();
            }
            else
            {
                this.nodes = s.getFunction();
            }
            // alert(JSON.stringify(this.nodes));
        }
    },
    findNode: function (value)
    {
        var self = this;
        var found = null;
        $.each(this.nodes, function (i, v)
        {
            if (v.value == value)
            {
                found = v;
            }

        });

        return found;
    },

    _drawNode: function (node, list, index)
    {
        var self = this;
        var li = $("<li>").attr("id", node.value);
        if (!node.selectable)
        {
            li.css({ "float": "left", "width": "25em" });
        }
        var cbx = null;
        var s = $.extend(true, {}, checkboxTree.fetchSettings, self.options);
        if (node.selectable)
        {
            var n = node.parent.value || node.value;
            if (node.parent.realname)
                n = node.parent.realname;
            if (node.realname)
                n = node.realname;


            if (n == "last_timestamp" || n == "created_date" || n == "estimated")
            {
                cbx = $('<input>').attr({ name: n, type: "radio", value: node.value }).addClass("filterControlCheckbox");

                if (!self.checkedValues[n])
                {
                    if (node.isdefault)
                    {
                        cbx.attr("checked", "checked");
                    }
                }
            }
            else
            {
                cbx = $('<input>').attr({ name: n, type: "checkbox", value: node.value }).addClass("filterControlCheckbox");
            }

            //if the value is currently in the filter, check it

            if (self.checkedValues)
            {
                var fv = self.checkedValues[n];

                if (fv)
                {
                    var fvals = fv.split(",");
                    if ($.inArray(node.value, fvals) >= 0)
                    {
                        cbx.attr("checked", "checked");

                        list.show();
                    }
                    else
                    {
                        cbx.removeAttr("checked");
                    }
                }
            }

            var a = $("<a>").text(node.name).addClass("checkboxLink");
            if (node.value == "currentsprint")
            {
                li.addClass("currentCheckbox");
                li.attr("title", "Filter by the current milestone. This value will change as the current milestone changes.");
            }
            else if (node.value == "currentuser")
            {
                li.addClass("currentCheckbox");
                li.attr("title", "Filter by the currently logged in user. This value will change if the logged in user changes.");
            }
            a.attr({ id: node.value, href: "#" }).click(function (e)
            {
                e.stopPropagation();
                e.preventDefault();

                cbx.click();
                cbx.change();
            });
            li.append(a);
            if (n == "assignee" || n == "reporter" || n == "verifier")
            {
                li.addClass("user");
            }
            else
                li.addClass(n);

            li.prepend(cbx);
            if (node.isinactive)
            {
                li.addClass("inactive");
                li.hide();
            }
            node.checkbox = cbx;

        }
        else
        {
            li.text(node.name).attr("id", node.value).addClass("parent");
        }
        list.append(li);
        if (node.nodes && node.nodes.length > 0)
        {
            li.addClass("parent").attr("id", node.value);

            if (node.value != "last_timestamp" && node.value != "created_date" && node.value != "estimated")
            {
                var tri = new threeStateSelector(0, node,
                {
                    onCheck: function ()
                    {
                        var children = li.find("input.filterControlCheckbox:not(:checked)");
                        children.click();
                        children.change();
                    },
                    onUncheck: function ()
                    {
                        var children = li.find("input.filterControlCheckbox:checked");
                        children.click();
                        children.change();
                    }

                });

                self.tristates["triImg" + node.value] = tri;
                node.listItem = li;
                li.prepend(tri.img);
            }
            var ul2 = $("<ul>");
            var right = $("<img>").attr("src", sgStaticLinkRoot + "/img/right_triangle_black.png").addClass("fc-toggle");

            var down = $("<img>").attr("src", sgStaticLinkRoot + "/img/down_triangle_black.png").addClass("fc-toggle");
            var plus = $("<span>");

            plus.html(down);
            plus.toggle(
                function ()
                {
                    li.removeClass("open");
                    ul2.hide();
                    plus.html(right);

                },
                function ()
                {
                    li.addClass("open");
                    plus.html(down);
                    ul2.show();
                });


            li.prepend(plus);

            $.each(node.nodes, function (index, val)
            {
                self._drawNode(val, ul2, index);
            });
            li.append(ul2);

        }
        else
        {
            li.prepend($("<span>").addClass("filterleaf"));
        }
    },

    draw: function (controlid, dialog)
    {
        var self = this;
        $('#' + controlid).remove();
        var nodes = [];
        var spanBlocking = $("<span>");
        var spanDepending = $("<span>");
        var spanKeyword = $("<span>").css("width", "400px");
        var keyword = $("<input>").css({ "width": "17em" }).addClass("searchinput").addClass("nobackgroundimg").attr({ "type": "text", "id": "inputFilterKeyword", "title": "Filter by text in item title, description or comment)" });
        var s = $.extend(true, {}, checkboxTree.fetchSettings, self.options);
        var divtoolbar = $("<div>").addClass("primary").addClass("outercontainer").addClass("filtertoolbar");
        var divKeyword = $("<div>").addClass("primary").attr("id", "divKeyword");
        spanKeyword.append($("<label>").text("Search Text:").attr("for", "keyword").css({ "padding-right": "5px" }))
        spanKeyword.append(keyword);
        divKeyword.append(spanKeyword);

        var blocking = $('<input>').attr({ name: "blocking", type: "checkbox", value: "true" }).attr("id", "blocking").addClass("filterControlCheckbox");
        spanBlocking.append(blocking);
        spanBlocking.append($('<label for="blocking">Blocking Items</label>'));
        divKeyword.append(spanBlocking);

        var depending = $('<input>').attr({ name: "depending", type: "checkbox", value: "true" }).attr("id", "depending").addClass("filterControlCheckbox");
        spanDepending.append(depending);
        spanDepending.append($('<label for="depending">Depending on Items</label>'));
        divKeyword.append(spanDepending);
        var div = $("<div>").attr("id", controlid).addClass("filterControlWrapper").addClass("clearfix");
        if (self.checkedValues && self.checkedValues["keyword"])
        {
            keyword.val(self.checkedValues["keyword"]);
        }
        if (self.checkedValues && self.checkedValues["blocking"])
        {
            blocking.attr("checked", "checked");
        }
        if (self.checkedValues && self.checkedValues["depending"])
        {
            depending.attr("checked", "checked");
        }
        if (!$("#toggleInactive").length)
        {
            var showUsers = $('<img>').attr({ "src": sgStaticLinkRoot + "/img/showuser.png", "title": "show inactive users" });
            var hideUsers = $('<img>').attr({ "src": sgStaticLinkRoot + "/img/hideuser.png", "title": "hide inactive users" })
            var toggleInactive = $('<a></a>')
            .html(showUsers)
            .addClass("toggleInactiveUsers")
            .attr("id", "toggleInactive")
            .toggle(
                function ()
                {
                    $(this).html(hideUsers);
                    $(".user.inactive").show();
                },
                function ()
                {
                    $(this).html(showUsers);
                    var cbxs = $(".user.inactive input:checked");
                    $.each(cbxs, function (i, c)
                    {
                        $(c).click();
                    });
                    $(".user.inactive").hide();
                });

            divtoolbar.append(toggleInactive);
        }

        if (!$("#toggleInactiveMiles").length)
        {
            var showMiles = $('<img>').attr({ "src": sgStaticLinkRoot + "/img/showmilestone.png", "title": "show released milestones" });
            var hideMiles = $('<img>').attr({ "src": sgStaticLinkRoot + "/img/hidemilestone.png", "title": "hide released milestones" })
            var toggleInactiveMiles = $('<a></a>')
            .html(showMiles)
            .addClass("toggleInactiveMiles")
            .attr("id", "toggleInactiveMiles")
            .toggle(
                function ()
                {
                    $(this).html(hideMiles);
                    $(".milestone.inactive").show();
                },
                function ()
                {
                    $(this).html(showMiles);
                    var cbxs = $(".milestone.inactive input:checked");
                    $.each(cbxs, function (i, c)
                    {
                        $(c).click();
                    });
                    $(".milestone.inactive").hide();
                });

            divtoolbar.append(toggleInactiveMiles);
        }

        var ul = $("<ul>").attr("id", "ul" + controlid).addClass("checkboxTree");

        nodes = this.nodes;
        $.each(nodes, function (i, node)
        {
            self._drawNode(node, ul, i);
        });

        this.stampsController = new stampFilterControl();
        ul.append(this.stampsController.draw());
        if (this.checkedValues["stamp"])
            this.stampsController.setStamps(this.checkedValues["stamp"].split(","));
        div.addClass("editable");

        div.append(divtoolbar);
        div.append(divKeyword);
        div.append(ul);

        dialog.html(div);
        $("input.filterControlCheckbox").change(function (e)
        {
            if (e.target.name != "last_timestamp" && e.target.name != "created_date")
            {
                e.stopPropagation();
                var parentList = $(e.target).parents("li.parent");

                self.setTriState(parentList);
            }
        });


        $.each(self.tristates, function (n, t)
        {
            self.setTriState(t.node.listItem, true);

        });

        var inactive_users = $(".user.inactive input:checked");

        if (inactive_users.length)
        {
            $("#toggleInactive").click();

        }

        var inactive_miles = $(".milestone.inactive input:checked");

        if (inactive_miles.length)
        {
            $("#toggleInactiveMiles").click();
        }


    },
    setTriState: function (parents, firstDraw)
    {
        var currentMileParentIds = [];

        if (sgCurrentSprint)
        {
            var currentMileNode = $("#ulfiltercontrol").find("#" + sgCurrentSprint) || null;

            if (currentMileNode)
            {
                var currentMileParents = currentMileNode.parents("li.parent");
                $.each(currentMileParents, function (i, v)
                {
                    currentMileParentIds.push($(v).attr("id"));
                });

            }
        }

        var self = this;
        $.each(parents, function (i, li)
        {
            var checksLength = $(li).find("input.filterControlCheckbox:checked").length;
            var allNodes = $(li).find("input.filterControlCheckbox");

            var name = $(li).attr("id");
            var tri = self.tristates["triImg" + name];
            if (tri)
            {
                if (checksLength == 0)
                {
                    tri.setState(0);
                    //hide milestone if nothing selected under it
                    if (firstDraw && $(allNodes[0]).attr("name") == "milestone")
                    {
                        if (currentMileParentIds.length)
                        {
                            if ($.inArray($(li).attr("id"), currentMileParentIds) < 0)
                            {
                                $($(li).find("img.fc-toggle")).click();
                            }
                        }
                        else
                        {
                            if ($(li).attr("id") != "(none)" && $(li).attr("id") != "milestone")
                            {
                                $($(li).find("img.fc-toggle")).click();
                            }
                        }
                    }

                }
                else if (checksLength == allNodes.length)
                {
                    tri.setState(1);
                }
                else
                {
                    tri.setState(2);
                }
            }
        });

    },

    getSelected: function ()
    {
        var self = this;
        var s = $.extend(true, {}, checkboxTree.fetchSettings, self.options);
        s.checkedValues = {};

        var selected = $("input.filterControlCheckbox:checked");

        var vals = {};

        $.each(selected, function (j, cbx)
        {
            if ($(this).val() != "any" && $(this).is(":visible"))
            {
                if (!vals[$(this).attr("name")])
                    vals[$(this).attr("name")] = [];

                vals[$(this).attr("name")].push($(this).val());
            }

        });

        var stamps = this.stampsController.stamps;
        if (stamps.length > 0)
        {
            vals["stamp"] = stamps;
        }

        $.each(vals, function (i, v)
        {
            s.checkedValues[i] = v.join(",");
        });

        if ($("#inputFilterKeyword").val())
        {
            s.checkedValues["keyword"] = $("#inputFilterKeyword").val();
        }

        return s.checkedValues;
    },
    setSelected: function (vals)
    {
        this.checkedValues = vals;
    }

});

function stampFilterControl()
{
	this.searchField = null;
	this.cloud = null;
	this.selected = false;
	this.stamps = [];
	this.errorFormatter = new bubbleErrorFormatter();
}

extend(stampFilterControl, {

    setStamps: function (stamps)
    {
        if (!stamps)
            return;

        if (typeof stamps == "string")
            stamps = [stamps];

        for (var i = 0; i < stamps.length; i++)
        {
            var stamp = stamps[i];

            var ele = this.renderStamp(stamp);

            this.cloud.append(ele);
            this.stamps.push(stamp);
        }
    },

    draw: function ()
    {
        var self = this;
        var stampsLi = $("<li>").attr("id", "stampsFilterList");
        stampsLi.css({ "float": "left", "width": "25em" });
        stampsLi.text("Stamps");

        var wrapper = $("<div>").attr("id", "filterStampsWrapper");
        this.cloud = $("<div>").attr("id", "filterStampCloud").addClass("clearfix");
        var formDiv = $("<div>").attr("id", "filterStampsForm");
        var right = $("<img>").attr("src", sgStaticLinkRoot + "/img/right_triangle_black.png");

        var down = $("<img>").attr("src", sgStaticLinkRoot + "/img/down_triangle_black.png");
        var plus = $("<span>");

        plus.html(down);
        plus.toggle(
			function ()
			{
			    stampsLi.removeClass("open");
			    wrapper.hide();
			    plus.html(right);

			},
			function ()
			{
			    stampsLi.addClass("open");
			    plus.html(down);
			    wrapper.show();
			}
		);

        stampsLi.prepend(plus);

        var searchLabel = $('<label>').attr("for", "search").text("Search:");
        var search = $("<input type='text' class='searchinput' maxlength='255' name='search'/>").attr('id', "filterStampSearch");
        search.autocomplete({
            minLength: 2,
            source: function (request, response)
            {
                self.errorFormatter.hideInputError(self.searchField);
                self.fetchStamps(request, response);
            },
            select: function (event, ui) { self.stampSelected(event, ui); },
            close: function (event, ui)
            {
                if (self.selected)
                {
                    $("#filterStampSearch").val("");
                    $("#filterStampSearch").focus();
                }
                self.searchField.field.removeClass("ui-autocomplete-loading");
                self.selected = false;
            },
            position: {
                my: "right top",
                at: "right bottom",
                collision: "none"
            },
            appendTo: stampsLi
        });
        search.keydown(function (event)
        {
            self.errorFormatter.hideInputError(self.searchField);
            if (event.keyCode == 27)
                event.stopPropagation();
        });
        search.keyup(function (event)
        {
            if (event.keyCode == 13)
            {
                self.addStamp($(event.target).val());
                $("#filterStampSearch").val("");
                search.autocomplete("close");
            }
            else if (event.keyCode == 27)
                $("#filterStampSearch").val("");
        });
        search.click(function (event)
        {
            if ($("#filterStampSearch").val() == "")
                return;

            search.autocomplete("search");
        });

        var searchError = this.errorFormatter.createContainer(search)
        this.searchField = new vFormField(null, null, search, null, searchError);

        formDiv.append(searchLabel);
        formDiv.append(search);
        formDiv.append(searchError);

        wrapper.append(formDiv);
        wrapper.append(this.cloud);

        stampsLi.append(wrapper);

        return stampsLi;
    },
    addStamp: function (stamp)
    {
        var self = this;

        if (!stamp)
            return;

        var error = this.validateStamp(stamp);

        if (error)
        {
            err = ["Stamps " + error];

            this.errorFormatter.displayInputError(this.searchField, err);
            return;
        }

        if ($.inArray(stamp, this.stamps) != -1)
            return;

        this.stamps.push(stamp);

        var ele = this.renderStamp(stamp);

        this.cloud.append(ele);
    },
    fetchStamps: function (request, response)
    {
        var self = this;

        var error = this.validateStamp(request.term);
        if (error)
        {
            var err = ["Stamps " + error];

            this.errorFormatter.displayInputError(this.searchField, err);
            this.searchField.field.removeClass("ui-autocomplete-loading");
            this.searchField.field.autocomplete("close");
            return;
        }

        vCore.ajax({
            url: vCore.repoRoot() + "/workitem/stamps.json?prefix=" + vCore.urlEncode(request.term),
            success: function (data)
            {
                if (data.error)
                {
                    err = ["invalid input - " + data.error];
                    self.errorFormatter.displayInputError(self.searchField, err);
                    self.searchField.field.removeClass("ui-autocomplete-loading");
                }

                response(data);
            }
        });
    },

    stampSelected: function (event, ui)
    {
        if (!ui || !ui.item || !ui.item.value)
            return;

        this.selected = true;
        this.addStamp(ui.item.value);
        $("#filterStampSearch").trigger("blur");

    },


    validateStamp: function (stamp)
    {
        //var noInvalidChars = /^[^,\\\/\r\n]*$/;

        return validate_object_name(stamp, get_object_name_constraints(CONSTRAINT_TYPE_WIT_STAMP));

    },

    renderStamp: function (stamp)
    {
        var self = this;
        var stampWrap = $("<div>").addClass("cont");
        var stamp = $("<span>").addClass("stamp").text(stamp);
        var deleteLink = $("<a>").addClass("delete").text("x");
        deleteLink.click(function () { self.deleteClicked(stampWrap); });

        stampWrap.append(stamp).append(deleteLink);

        return stampWrap;
    },

    deleteClicked: function (stampWrap)
    {
        var stamp = $(".stamp", stampWrap).text();
        var index = $.inArray(stamp, this.stamps);
        if (index != -1)
        {
            this.stamps.splice(index, 1);
        }

        stampWrap.remove();
    }

});

function findSprint(sprints, recid)
{
    for (var i = 0; i < sprints.length; i++)
    {
        if (sprints[i].recid == recid)
            return sprints[i];
    }
    return null;
}

function createSprintNode(sprint, currentSprint, node, sprints)
{
    //(new checkboxTreeNode(name, v.recid, true, null, null, false, v.inactive)
    var active = !sprint.releasedate;
    var name = sprint.name;
    if (!active)
    {
        name += " (released)";
    }
    var newNode = new checkboxTreeNode(name, sprint.recid, true, null, null, false, !active );
    newNode.realname = "milestone";
  
    node.addNode(newNode);
    if (sprint.children && sprint.children.length > 0)
    {          
        var children = [];
        $.each(sprint.children, function (n, c)
        {
            var spr = findSprint(sprints, c);
            children.push(spr);
        });
        children.sort(function (a, b)
        {
            return (a.startdate - b.startdate);
        });
        $.each(children, function (index, child)
        {
            createSprintNode(child, currentSprint, newNode, sprints);
        });
    }
}




