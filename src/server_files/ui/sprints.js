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

var controller;

function MilestoneTableNode(mileNode, tr, parentControl)
{
    this.mileNode = mileNode;
    this.tableRow = tr;
    this.expanded = true;
    this.expandedChildren = [];
    this.children = [];
    this.parent = null;
    this.handle = null;
    this.parentControl = parentControl;
    
}
extend(MilestoneTableNode, {

    anyCollapsedParents: function ()
    {
        var node = this;
        while (node != null)
        {
            if (!node.expanded)
            {
                return true;
            }
            node = node.parent;
            if (node == null)
                break;
        }
        return false;
    },
    addChild: function (node)
    {
        this.children.push(node);
        node.parent = this;
    },
    expandChildren: function (nodes)
    {
        var self = this;
        $.each(nodes, function (i, v)
        {
            if (self.parentControl.showReleased == true)
            {
                v.tableRow.show();
            }
            else
            {
                var releaseDate = null;
                if (v.mileNode.data)
                    releaseDate = this.mileNode.data.releasedate;
                if (!releaseDate)
                {
                    v.tableRow.show();
                }

            }
            if (v.expanded)
            {
                v.expandChildren(v.children);
            }

        });
    },
    expand: function (keepExpandState)
    {
        var self = this;

        if (!keepExpandState)
        {
            this.expanded = true;
            if (this.handle)
            {
                this.handle.removeClass("closed");
                this.handle.addClass("open");
            }
        }
        this.expandChildren(this.children);

    },
    collapse: function (keepCollapseState)
    {
        if (!keepCollapseState)
        {
            this.expanded = false;
            if (this.handle)
            {
                this.handle.addClass("closed");
                this.handle.removeClass("open");
            }
        }

        $.each(this.children, function (i, v)
        {
            v.tableRow.hide();
            v.collapse(true);
        });

    }
});

function SprintTableControl(containter, options)
{
    this.container = containter || null;   
    this.milestoneTree = new vCore.milestoneTree();
    this.showReleased = false;

    this.options = jQuery.extend(
	{
	    beforeSprintSelect: null,
	    onSelect: null,
	    selectCurrent: false,
	    closeReleased: false

	}, (options || {}));

    this._initControl();
}

extend(SprintTableControl, {
    currentSprint: null,
    rootNode: null,

    _initControl: function (recid)
    {
        var self = this;

        $(this).hide();
        var params = $.deparam.fragment();

        if (params["released"] == "true")
        {
            this.showReleased = true;
        }

        $('#imgToggleReleasedMiles').unbind("toggle");
        $('#imgToggleReleasedMiles').click(
                function (e)
                {
                    self.showReleased = !self.showReleased;
                    if (self.showReleased)
                    {
                        $(this).attr({ "src": sgStaticLinkRoot + '/img/hidemilestone.png', "title": "hide released milestones" });

                    }
                    else
                    {
                        $(this).attr({ "src": sgStaticLinkRoot + '/img/showmilestone.png', "title": "show released milestones" });
                    }
                  
                    $.bbq.pushState({ "released": self.showReleased });
                    $(window).trigger('hashchange');
                });
        $(window).bind('hashchange',
            function ()
            {
                self.draw()
            });
        this._fetchSprints();

    },

    _fetchSprints: function ()
    {
        var self = this;
        vCore.fetchSprints({
            includeLinked: false,
            force: true,
            success: function (data, status, xhr)
            {
                self._setSprints(data);
            }
        });

    },
    _setSprints: function (data)
    {
        var self = this;
        this.milestoneTree.reset();
        $(data).each(function (i, v)
        {
            self.milestoneTree.insertMilestone(v)
        });
        if (!this.currentSprint)
        {
            vCore.ajax({
                url: vCore.repoRoot() + "/config/current_sprint",
                dataType: 'json',
                success: function (data, status, xhr)
                {
                    self.currentSprint = data.value;

                    self.draw();
                }
            });
        }
        else
        {
            this.draw();
        }
    },

    draw: function ()
    {
        if (this.container)
            this.container.empty();

        if (this.milestoneTree.length > 0)
        {
            $('#imgToggleReleasedMiles').show();

        }
        else
        {
            $('#imgToggleReleasedMiles').hide();

        }
        //        this.container.append($("<br/>"));
        var table = $("#tableMilestones");
        this._drawNode(table, "", this.milestoneTree.root);

        // this.container.addClass("sprintTreeControl");
        //this.container.append(ul);

        this.bindObservers();

    },
    _drawNode: function (cont, parentTableNode, node)
    {
        var self = this;

        var tr = $("<tr>").addClass("sprint trReader");
        var td = $("<td>");
        var mileID = "productbacklog";
        if (node.data)
        {
            tr.attr("id", node.data.recid);
            mileID = node.data.recid;
        }

        if (this.currentSprint && this.currentSprint == mileID)
        {
            tr.css({ "font-weight": "bold" });
        }

        var a = $("<a>").attr({ "href": vCore.repoRoot() + "/workitems.html?milestone=" + mileID + "&groupedby=status&columns=id%2Ctitle%2Cassignee%2Creporter%2Cpriority%2Cmilestone%2Clogged_time%2Cremaining_time&maxrows=100?skip=0",
            "title": "View items in this milestone"
        }).text(node.title);
        var title = $("<span>").addClass("title").append(a);

        if (node.data && node.data.releasedate)
        {
            title.addClass("released");

            tr.addClass("li-released")
            if (!this.showReleased)
                tr.hide();
            a.text(a.text() + " (released " + shortDate(new Date(node.data.releasedate)) + ")");
        }
        var tdcontext = $("<td>");
        if (node.data && (node.data.recid == this.currentSprint))
        {
            title.addClass("current");
            tdcontext.addClass("default");
        }

        td.append(title).addClass("name");
        var indent = node.numParents() * 20;
        td.css({ "padding-left": indent + "px" });
        tr.append(td);
        if (node.data)
        {
            var parent = this.milestoneTree.root.title;
            if (node.data.parent)
            {
                parent = this.getSprint(node.data.parent).name;
            }

            var startDateString = "";
            var endDateString = "";
            var startDate = null;
            var endDate = null;
            var days = "";
            if (node.data.startdate)
            {
                startDate = new Date(node.data.startdate);
                startDateString = shortDate(startDate);
            }
            if (node.data.enddate)
            {
                endDate = new Date(node.data.enddate);
                endDateString = shortDate(endDate);
            }
            tr.append($("<td>").text(node.data.description).addClass("description"));
            tr.append($("<td>").text(startDateString).addClass("startdate"));
            tr.append($("<td>").text(endDateString).addClass("enddate"));
            if (node.data.startdate && node.data.enddate)
            {
                days = vCore.workingDays(startDate, endDate);
            }
            tr.append($("<td>").text(days).addClass("workingdays"));
        }
        else
        {

            tr.append($("<td colspan=4></td>"));

        }
        tr.append($("<td>"));
        cont.append(tr);

        var spacer = $('<div>').addClass("spacer");
        tdcontext.append(spacer);
        var span = $("<span>").addClass("linkSpan");
        if (!isTouchDevice())
            span.hide();
        var img = $(document.createElement('img')).attr('src', sgStaticLinkRoot + "/img/contextmenu.png");
        var imgEdit = $(document.createElement('img')).attr('src', sgStaticLinkRoot + "/img/edit.png");
        var a = $("<a>").html(img).attr({ "href": "#", "title": "view context menu", "id": "context" + mileID }).addClass("small_link");
        var aEdit = $("<a>").html(imgEdit).attr({ "href": "#", "title": "edit mode", "id": "edit" + mileID }).data("mileid", mileID).addClass("small_link");
        if(mileID=="productbacklog"){
                aEdit.css({visibility: "hidden"});
        }
        span.append(aEdit);
        span.append(a);
        a.click(function (e)
        {
            e.preventDefault();
            e.stopPropagation();

            if (node.data && node.data.releasedate)
            {
                $(".norel").hide();
            }
            else if (mileID == "productbacklog")
            {
                $(".nobl").hide();
            }
            else
            {
                $(".nobl").show();
                $(".norel").show();
            }
            showCtxMenu($(this).attr("id"), $("#milestonecontextmenu"), { "recid": mileID });
        });
        aEdit.click(function (e)
        {
            e.preventDefault();
            e.stopPropagation();

            var sprint = controller.tableController.getSprint(mileID);
            controller._editRow(sprint);
            return false;
        });

        tdcontext.append(span);
        tr.prepend(tdcontext);

        var unreleasedCount = 0;
        var tableNode = new MilestoneTableNode(node, tr, this);
        if (parentTableNode)
        {
            parentTableNode.addChild(tableNode);
        }
        if (node.children.length > 0)
        {
            td.addClass("hasChildren");

            var handle = $("<a>").addClass("disclosure open").data("recid", mileID);
            handle.unbind("click");
            handle.click(function ()
            {
                if ($(this).hasClass("open"))
                {
                    tableNode.collapse();
                }
                else
                {
                    tableNode.expand();
                }

            });
            tableNode.handle = handle;
            td.prepend(handle);
            if (this._anyUnreleased(node.children) == false && this.showReleased == false)
            {
                handle.addClass("nolivekids clearbackground");
            }
            else
            {
                handle.removeClass("clearbackground");
            }
            $.each(node.children, function (i, v)
            {
                self._drawNode(cont, tableNode, v);
            });
            if (node.data && node.data.releasedate && this.options.closeReleased)
                tr.hide();
        }
        else
        {
            var handle = $("<span>").addClass("disclosure clearbackground");
            td.prepend(handle);
            tableNode.handle = handle;
        }
        if (this.milestoneTree.length == 0)
        {
            cont.append('<tr id="trnomilestone" class="trReader nohover"><td colspan="7"><b>No milestones are configured. Click the Add Milestone link to add a new Milestone.</b></td></tr>');
        }

    },
    _anyUnreleased: function (children)
    {
        for (var i = 0; i < children.length; i++)
        {
            if (!children[i].data.releasedate)
                return true;

        }
        return false;
    },
    getSprints: function ()
    {
        return this.milestoneTree.getMilestones();
    },
    getSprint: function (recid)
    {
        return this.milestoneTree.getMilestone(recid).data;
    },

    refresh: function (recid)
    {
        this._fetchSprints();
        this.draw();
    },

    bindObservers: function ()
    {
        enableRowHovers();
    }

});

function sprintFormController()
{
    this.tableController = null;
	this.tableController = null;
	this.loading = null;
	this.form = null;	
	this.currentSprint = sgCurrentSprint;
	this.sprints = [];
	this.addDialog = null;
	this.releaseSprint = null;
	this.sprintItems = null;
	this.selectedSprint = null;
	this.parentID = null;
	this.releaseDialog = null;
	this.releaseForm = null;

	var self = this;

	this.init = function ()
	{
	    var self = this;

	    this.releaseDialog = $("<div>").addClass("primary editable innercontainer").attr("id", "releaseDialog")
            .vvdialog({
                buttons:
                        {
                            "Release": function ()
                            {
                                self.releaseForm.submitForm();
                            },
                            "Cancel": function ()
                            {
                                self.releaseSprint = null;
                                self.selectedSprint = null;
                                if (self.releaseForm)
                                {
                                    self.releaseForm.reset();
                                }
                                $(this).vvdialog("vvclose");
                            }
                        },
                close: function (event, ui)
                {
                    $("#backgroundPopup").hide();
                },
                open: function (event, ui)
                {
                    if (self.releaseForm)
                    {
                        delete self.releaseForm;
                        self.releaseForm = null;
                    }
                    $("#releaseDialog").empty();
                    self.releaseSelectedSprint();
                },
                autoOpen: false

            });
	    this.addDialog = $("<div>").addClass("primary editable innercontainer").attr("id", "sprintForm")
		            .vvdialog({
		                //		                position: [$("#link_columns").offset().left, $("#link_columns").offset().top + 20],

		                resizable: false,
		                width: 500,
		                height: 450,
		                buttons:
                        {
                            "Save": function ()
                            {
                                self.form.submitForm();
                            },
                            "Cancel": function ()
                            {
                                self.form.reset();
                                self.parentID = null;
                                $(this).vvdialog("vvclose");
                            }
                        },
		                open: function (event, ui)
		                {
		                    if (self.form)
		                    {
		                        delete self.form;
		                        self.form = null;
		                    }

		                    if (self.selectedSprint)
		                    {
		                        var sprint = self.tableController.getSprint(self.selectedSprint);

		                        var vFormOptions = jQuery.extend(true, {
		                            url: vCore.repoRoot() + "/milestone/" + sprint.recid + ".json",
		                            method: "PUT",
		                            handleUnload: true
		                        }, self._formOptions);

		                        self.form = new vForm(vFormOptions);
		                        self.form.setRecord(sprint);
		                    }
		                    else
		                    {
		                        var vFormOptions = jQuery.extend(true,
			                    {
			                        url: vCore.repoRoot() + "/milestones.json",
			                        method: "POST",
			                        handleUnload: true
			                    }, self._formOptions);
		                        self.form = new vForm(vFormOptions);
		                    }


		                    self.renderForm(self.form, self.parentID);
		                },
		                close: function (event, ui)
		                {
		                    $("#backgroundPopup").hide();
		                },
		                autoOpen: false,
		                title: 'Add Milestone'
		            });




	    this.tableController = new SprintTableControl($("#tbodyMilestones"), {

	        beforeSprintSelect: function () { return self.sprintWillChange(); }

	    });
	    this._fetchCurrent();
	    $("#linkNewMilestone").unbind("click");
	    $("#linkNewMilestone").click(function (e)
	    {
	        e.preventDefault();
	        e.stopPropagation();

	        self._cancelFormEdits();
	        checkUnload = true;
	        self.addDialog.vvdialog("vvopen");

	    });
	    $("#milestonecontextmenu li").unbind("click");
	    $("#milestonecontextmenu li").click(function (e)
	    {
	        self.selectedSprint = $(this).data('recid');
	        self.contextMenuClicked(e, $(this));
	    });
	};

	this._fetchCurrent = function(after)
	{
		vCore.ajax( {
			url : vCore.repoRoot() + "/config/current_sprint",
			dataType: 'json',
			success: function(data, xhr, status) {
				if (data)
					self.currentSprint = data.value;
				if (after)
					after(data.value);
			}
		});
	};

	this._formOptions = {
	    rectype: "sprint",
	    errorFormatter: bubbleErrorFormatter,
	    beforeFilter: function (form, rec)
	    {
	        var startdate = form.getRecordValue("startdate");
	        if (!rec.startdate)
	        {
	            if (startdate != null || startdate != undefined)
	            {
	                rec.startdate = null;
	            }
	            else
	            {
	                delete rec.startdate;
	            }
	        }
	        var enddate = form.getRecordValue("startdate");
	        if (!rec.enddate)
	        {
	            if (enddate != null || enddate != undefined)
	            {
	                rec.enddate = null;
	            }
	            else
	            {
	                delete rec.enddate;
	            }
	        }

	        return rec;
	    },
	    customValidator: function (form) { return self.validate(form); },
	    afterSubmit: function (form)
	    {
	        self.addDialog.vvdialog("vvclose");
	        self.parentID = null;
	        self.selectedSprint = null;
	        self.tableController.refresh(form.getRecordValue("recid"));
	    }
	};
	

	this.sprintWillChange = function()
	{
		if (this.form)
		{
			if (this.form.willBeDestroyed() === false)
				return false;

			delete this.form;
			this.form = null;
		}

		return true;
	};

	this.renderForm = function (form, parentSprint, showImageButtons)
	{
	    if (!form)
	        return;
	    var divForm = $("<div>");
	    form.ready(false);
	    var invalidParents = [];
	    var sprints = this.tableController.getSprints();
	    var recid = null;

	    if (form.record)
	    {
	        recid = form.record.recid;
	    }

	    invalidParents = this._buildInvalidParents(sprints, form.record, invalidParents);

	    // Name
	    var divName = $("<div>").attr("id", "div_name").addClass("divEditItem");

	    var nameOpts = { validators: { required: true} };
	    if (!showImageButtons)
	        nameOpts.label = "Name";
	    var nameField = form.field(divName, "text", "name", nameOpts);
	    divForm.append(divName);

	    // Description
	    var divDescription = $("<div>").attr("id", "div_description").addClass("divEditItem"); ;

	    var descOpts = {
	        rows: 3
	    };
	    if (!showImageButtons)
	        descOpts.label = "Description";
	    var descField = form.field(divDescription, "textarea", "description", descOpts);
	    divForm.append(divDescription);

	    // Parent Sprint
	    var divParent = $("<div>").attr("id", "div_parent");
	    divForm.append(divParent);
	    var parentOpts = {
	        // label: "Parent",
	        nested: true,
	        filter: function (value)
	        {
	            if (value)
	                value = value.recid;

	            if ($.inArray(value, invalidParents) > -1)
	                return false;

	            return true;
	        }
	    };
	    if (!showImageButtons)
	    {
	        parentOpts.label = "Parent";
	        parentOpts.wrapField = true;
	        parentOpts.isBlock = true;
	    }
	    var sprintSelect = form.field(divParent, "sprintSelect", "parent", parentOpts);
	    divForm.append(divParent);

	    var dateDiv = $(document.createElement("div"));
	    dateDiv.attr({
	        id: "dates"
	    }).addClass("clearfix");

	    // Start Date	   
	    var startDiv = $(document.createElement("div")).attr("id", "div_startdate").addClass("divEditItem"); ;
	    var sdOpts = {
	        onchange: function (input) { self.sprintDatesDidChange(input, showImageButtons); }
	    };
	    if (!showImageButtons)
	        sdOpts.label = "Start Date";
	    var startDateField = form.field(startDiv, "dateSelect", "startdate", sdOpts);
	    dateDiv.append(startDiv);

	    // End Date
	    var endDiv = $(document.createElement("div")).attr("id", "div_enddate").addClass("divEditItem");
	    var edOpts = {
	        //  label: "End Date",
	        onchange: function (input) { self.sprintDatesDidChange(input, showImageButtons); }
	    };
	    if (!showImageButtons)
	        edOpts.label = "End Date";
	    var endDateField = form.field(endDiv, "dateSelect", "enddate", edOpts);

	    dateDiv.append(endDiv);

	    divForm.append(dateDiv);

	    var workingDays = $(document.createElement("div"));
	    workingDays.attr("id", "workingDays");
	    $("#sprintForm").append(workingDays);

	    var buttons = $(document.createElement("div"));
	    buttons.attr("id", "buttons").addClass("clearfix");
	    var right = $(document.createElement("div")).css("float", "right");
	    var left = $(document.createElement("div")).css("float", "left");

	    // Submit and Cancel
	    var releasedate = form.getRecordValue("releasedate");
	    if (releasedate)
	    {
	        var date = new Date(parseInt(releasedate));
	        var fdate = vCore.formatDate(date);
	        var rdiv = $("<div>").text("Released: " + fdate);
	        left.append(rdiv);
	        startDateField.disable();
	        endDateField.disable();
	    }
	    else
	    {
	        $("#sprint_startdate").removeAttr("disabled");
	        $("#sprint_enddate").removeAttr("disabled");
	        if (form.record)
	        {
	            if (this.currentSprint == form.getRecordValue("recid"))
	            {
	                this.addDialog.vvdialog("option", "title", form.getRecordValue("name") + " (Current Milestone)");
	            }

	        }
	    }
	    buttons.append(right).append(left);
	    divForm.append(buttons);

	    if (showImageButtons)
	    {

	        var divButtons = $("<div>").attr("id", "divEditButtons").addClass("divEditItem");
	        divButtons.css("display", "none");

	        var saveButton = form.imageButton("imgButtonSaveEdits", sgStaticLinkRoot + "/img/save16.png");
	        saveButton.addClass("filterSaveButton");
	        saveButton.click(function (e)
	        {
	            form.submitForm();
	        });

	        var cancelButton = form.imageButton("imgButtonCacnelEdits", sgStaticLinkRoot + "/img/cancel18.png");
	        cancelButton.addClass("filterCancelButton");
	        cancelButton.click(function (e)
	        {
	            self._cancelFormEdits();
	        });
	        divButtons.append(saveButton);
	        divButtons.append(cancelButton);
	        divForm.append(divButtons);
	    }
	    this.addDialog.html(divForm);
	    form.ready();
	    if (parentSprint)
	    {
	        $("#sprint_parent option[value=" + parentSprint + "]").attr("selected", "selected");

	    }

	};

	this.sprintDatesDidChange = function (input, hidewdlabel)
	{

	    if (input.attr)
	        var name = input.attr("name");

	    var startId = this.form._generateId("startdate");
	    var endId = this.form._generateId("enddate");
	    var startDate = $("#" + startId).datepicker('getDate');
	    var endDate = $("#" + endId).datepicker('getDate');

	    var sdfield = this.form.getField("startdate");
	    var edfield = this.form.getField("enddate");

	    this.form.hideErrors();

	    if (!startDate && !endDate)
	    {
	        $("#workingDays").text("");
	        return;
	    }

	    if (name == "startdate" && startDate && !endDate)
	    {
	        $("#" + endId).datepicker("setDate", startDate);
	        endDate = startDate;
	    }

	    if (name == "enddate" && endDate && !startDate)
	    {
	        $("#" + startId).datepicker("setDate", endDate);
	        startDate = endDate;
	    }

	    if (!this.form.validate())
	    {
	        var nameInp = this.form.getField("name");
	        this.form.displayErrors();
	    }
	    else
	    {
	        var days = vCore.workingDays(startDate, endDate);
	        if (hidewdlabel)
	        {
	            $("#workingDays").text(days);
	        }
	        else
	        {
	            $("#workingDays").text(days + " Working Days");
	        }
	    }
	};

	this.validate = function(form)
	{
		var record = form.record;
		var newRec = form.getFieldValues();

		// Get the field we might need to add errors to
		var sdfield = form.getField("startdate");
		var edfield = form.getField("enddate");
		var nameField = form.getField("name");
		
		var valid = true;

		if (newRec.startdate && ! newRec.enddate)
		{
			valid = false;

			form.addError(edfield, 'An end date is needed when a start date is given')
		}
		else if (newRec.enddate && ! newRec.startdate)
		{
			valid = false;
			form.addError(sdfield, 'A start date is needed when an end date is given');
		}
		else if (newRec.enddate < newRec.startdate)
		{
			valid = false;
			//form.addError(sdfield, "Milestone start date can't be later than the end date");
			form.addError(edfield, "Milestone end date can't be earlier than the start date")
		}

		if (newRec.name.toLowerCase() == "product backlog")
		{
			valid = false;
			form.addError(nameField, "\"" + newRec.name + "\" is not a valid name");
		}
		
		var precid = null;
		if (newRec.parent)
			precid = newRec.parent;
			
		var parent = this.tableController.milestoneTree.getSubTree(precid);
		
		$.each(parent.root.children, function(i, node) {
			if (node.data.recid == form.getRecordValue("recid"))
				return;
				
			if (node.data.name == newRec.name)
			{
				valid = false;
				form.addError(nameField, "The milestone name is not unique for parent " + parent.root.title + ", please pick a new name");
			}
		});
		
		var parentId = this.form._generateId("parent");
		var parent_recid = $("#" + parentId).val();
		var parent = this.tableController.getSprint(parent_recid);

		if (parent && parent.children && record)
		{
			$.each(parent.children, function(i,node) {
				var sprint = node.data;
				if (!sprint)
					return;

				if (sprint.recid == record.recid)
					return;

				var _sstart = new Date(parseInt(sprint.startdate));
				var _send = new Date(parseInt(sprint.enddate));

				if ((newRec.startdate >= _sstart.getTime() && 
					newRec.startdate <= _send.getTime()) ||
					(newRec.enddate >= _sstart.getTime() &&
					newRec.enddate <= _send.getTime())
					)
				{
					valid = valid && confirm("Milestone dates overlap with milestone " + sprint.name + "\n Save anyway?");
					return false; // break loop
				}	

			});
		}
		return(valid);
	};
	this.contextMenuClicked = function (e, li)
	{
	    e.stopPropagation();
	    e.preventDefault();

	    this._cancelFormEdits();
	    var self = this;
	    var action = li.attr("action");
	    var recid = $("#milestonecontextmenu").data("recid");
	    this.selectedSprint = recid;
	    var sprint = this.tableController.getSprint(recid);

	    switch (action)
	    {
	        case "release":
	            //this.releaseSprintClicked();
	            this.releaseDialog.vvdialog("option", "title", "Release Sprint: " + sprint.name);
	            this.releaseDialog.vvdialog("vvopen");
	            break;
	        case "setcurrent":
	            utils_savingNow("Setting current milestone...");
	            $("#milestonecontextmenu").hide();
	            vCore.setConfig("current_sprint", recid, {
	                success: function ()
	                {
	                    self._fetchCurrent(function (current)
	                    {
	                        self.tableController.currentSprint = current;
	                        self.tableController.refresh();
	                        utils_doneSaving();
	                    });

	                }
	            });
	            break;
	        case "addchild":
	            checkUnload = true;
	            this.selectedSprint = null;
	            this.parentID = recid;
	            this.addDialog.vvdialog("vvopen");
	            break;
	        case "changeparent":
	            this.showChangeParentMenu(sprint);
	            break;

	    }

	},
    this._cancelFormEdits = function ()
    {
        checkUnload = false;

        this.parentID = null;
        this.selectedSprint = null;
        if (this.form)
        {
            delete this.form;
            this.form = null;
        }
        $("#sprintForm").empty();
        if (this.tableController.showReleased == true)
        {
            $(".trReader").show();
        }
        else
        {
            $(".trReader:not(.li-released)").show();
        }
        $(".trEdit").hide();

        $(".bubbleError").hide();
        enableRowHovers();

    },

    this._editRow = function (sprint)
    {

        var self = this;
        $(".divEditItem").remove();

        $("#milestonecontextmenu").hide();
        $("#divParentSubmenu").hide();
        this._cancelFormEdits();
        checkUnload = true;
        this.addDialog.vvdialog("vvclose");

        var tr = $("#" + sprint.recid);
        var trEdit = $("<tr>").addClass("trEdit");
        trEdit.append($("<td>").addClass("spacer hovered"));
        if (this.form)
        {
            delete self.form;
            self.form = null;
        }

        $("#sprintForm").empty();

        var vFormOptions = jQuery.extend(true, {
            url: vCore.repoRoot() + "/milestone/" + sprint.recid + ".json",
            method: "PUT",
            handleUnload: true
        }, this._formOptions);

        this.form = new vForm(vFormOptions);
        this.form.ready(false);
        this.form.setRecord(sprint);

        this.renderForm(this.form, null, true);

        // this.form.ready(true);
        var rowHeight = 24;
        var topPadding = 0;

        var columns = ["name", "description", "startdate", "enddate"];
        $.each(columns, function (i, v)
        {
            var td = tr.find("td." + v);
            var tdEdit = td.clone();
            trEdit.append(tdEdit);

            if (td && td.length)
            {
                var field = self.form.fieldsHash[v];

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
                //var div = $("<div>").append(field);
                var div = $("#div_" + v);
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

        var startDate = new Date(sprint.startdate);
        var endDate = new Date(sprint.enddate);
        var divWorkingDays = $("<div>").attr("id", "workingDays").text(vCore.workingDays(startDate, endDate));
        trEdit.append($("<td>").append(divWorkingDays).addClass('hovered'));
        //var divButtons = $("<div>").attr("id", "divEditButtons").addClass("divEditItem");
        var deb = $("#divEditButtons");
        deb.css("white-space", "nowrap").show();
        trEdit.append($("<td>").append(deb).addClass('hovered'));
        tr.after(trEdit);
        tr.hide();

        $("#sprint_name").focus();
        $(".divEditItem").unbind("keydown");
        $(".divEditItem").keydown(function (e)
        {
            if (e.which == 13)
            {
                e.preventDefault();
                e.stopPropagation();
                self.form.submitForm();
            }
            else if (e.which == 27)
            {
                e.preventDefault();
                e.stopPropagation();
                tr.find("td.hovered").removeClass("hovered");

                self._cancelFormEdits();

            }

        });

    },
    this.showChangeParentMenu = function (sprint)
    {
        var self = this;
        $(".lichangeparent").remove();
        var invalidParents = [];
        var sprints = this.tableController.getSprints();
        var recid = sprint.recid;

        invalidParents = this._buildInvalidParents(sprints, sprint, invalidParents);

        sprints.unshift({ "name": this.tableController.milestoneTree.root.title, "recid": "" });
        $.each(sprints, function (i, v)
        {

            if (v.recid != recid && v.recid != sprint.parent && !v.releasedate && ($.inArray(v.recid, invalidParents) < 0))
            {
                var li = $("<li>").text(v.name).addClass("lichangeparent");
                li.data("recid", recid);
                li.data("parentid", v.recid);
                li.click(function (e)
                {
                    sprint.parent = $(this).data("parentid");
                    vCore.ajax(
		            {
		                url: vCore.repoRoot() + "/milestone/" + recid + ".json",
		                method: "PUT",
		                dataType: 'json',
		                contentType: 'application/json',
		                type: 'PUT',
		                data: JSON.stringify(sprint),

		                success: function ()
		                {
		                    self.init();
		                    $("#divParentSubmenu").hide();
		                    $("#milestonecontextmenu").hide();
		                }
		            }
	            );


                });
                $("#ulContextParents").append(li);
            }

        });

        showContextSubMenu(recid, $("#liChangeParent"), $("#divParentSubmenu"));
    },
    this.releaseSelectedSprint = function ()
    {
        var self = this;
        var toReleaseTree = this.tableController.milestoneTree.getSubTree(this.selectedSprint);
        var div = $("#releaseDialog");
        unreleased = [];
        for (var i = 0; i < toReleaseTree.root.children.length; i++)
        {
            var node = toReleaseTree.root.children[i];
            if (!node.data.releasedate)
                unreleased.push(node.data);
        }

        if (unreleased.length > 0)
        {
            var p = $("<p>").text("Unable to release milestone. The following milestones are still open.");
            div.append(p);

            var names = $.map(unreleased, function (v) { return vCore.htmlEntityEncode(v.name); });
            var list = $("<p>").html(names.join("<br/>"));
            div.append(list);

        }
        else
        {
            this.loading = new smallProgressIndicator("releaseLoading", "Loading milestone...");
            div.append(this.loading.draw());
            var queue = new ajaxQueue({
                onSuccess: function ()
                {
                    self._displaySprintReleaseForm();
                }
            });

            queue.addQueueRequest({
                url: vCore.getOption("currentRepoUrl") + "/milestone/" + this.selectedSprint + ".json",
                dataType: 'json',
                success: function (data)
                {
                    self.releaseSprint = data;
                }
            });

            queue.addQueueRequest({
                url: vCore.getOption("currentRepoUrl") + "/milestone/" + this.selectedSprint + "/items.json",
                dataType: 'json',
                success: function (data)
                {
                    self.sprintItems = data;
                }
            });

            queue.execute();
        }

    },

	this._displaySprintReleaseForm = function ()
	{
	    if (!this.releaseSprint)
	    {
	        return;
	    }

	    var div = $("#releaseDialog");
	    var sprint = this.releaseSprint;
	    var items = [];

	    if (this.sprintItems)
	    {
	        items = this.sprintItems;
	    }


	    // Add a call back to the master form so we can blank the form on submit
	    // this.form.options.afterValidation = function (form, valid) { return self._formWillSubmit(form, valid); }

	    var options = {
	        rectype: "release",
	        forceDirty: true,
	        url: vCore.repoRoot() + "/milestone/release/" + sprint.recid + ".json",
	        method: "PUT",
	        smallLabels: true,
	        afterSubmit: function () { self._sprintReleased(); }
	    };

	    this.releaseForm = new vForm(options);
	    var rec = {
	        rectype: "release",
	        move: "true"
	    };
	    this.releaseForm.setRecord(rec);

	    var openCount = 0;
	    $.each(items, function (i, v)
	    {
	        if (v.status == "open")
	            openCount++;
	    });


	    if (openCount != 0)
	    {
	        var open = $("<span>");
	        var openLink = $("<a>").text(openCount + " incomplete work items were found.");
	        openLink.attr({
	            href: vCore.repoRoot() + "/workitems.html?milestone=" + sprint.recid + "&status=open&sort=priority&groupedby=status&columns=id%2Ctitle%2Cassignee%2Creporter%2Cpriority%2Cmilestone&maxrows=100?skip=0",
	            target: "_new"
	        });
	        open.append(openLink);
	        div.append(open);

	        var move = this.releaseForm.radio("move", "true").attr("id", "sprint_release_move");
	        var ignore = this.releaseForm.radio("move", "false").attr("id", "sprint_release_ignore");

	        var table = $("<table>");

	        // first row
	        var row = $("<tr>");
	        row.append($("<td>").attr({ valign: "top" }).append(ignore));
	        row.append($("<td>").append(this.releaseForm.label("move", "Ignore them").attr("for", ignore.attr("id"))));
	        table.append(row);

	        // second row
	        row = $("<tr>");
	        var td = $("<td>").attr({ "valign": "top" }).append(move);
	        row.append(td);
	        var td = $("<td>");
	        td.append(this.releaseForm.label("move", "Move them into:").attr("for", move.attr("id"))).append("<br/>");

	        td.append(this.releaseForm.sprintSelect("new_sprint", {
	            //allowEmptySelection: true,
	            emptySelectionText: "(Product Backlog)",
	            nested: true,
	            filter: function (sprint)
	            {
	                var selected = self.releaseSprint.recid;

	                if (!sprint || (!sprint.releasedate && (sprint.recid != selected)))
	                    return true;
	            }
	        }));

	        row.append(td);
	        table.append(row);

	        row = $('<tr />');
	        td = $('<td>').
				appendTo(row);
	        td.append(this.releaseForm.booleanCheckbox("set_current").attr('disabled', true));
	        td = $('<td>').
				appendTo(row);

	        td.append(this.releaseForm.label("set_current", "Mark Current"));
	        table.append(row);


	        div.append(table);
	    }
	    else
	    {
	        var confirm = $("<span>").text("No incomplete items.");
	        div.append(confirm);
	        div.append($("<br/>"));
	    }

	    var buttons = $("<div>").attr("id", "release_buttons");


	    /*  var release = this.form.submitButton("Release");
	    release.css("float", "right");

	    buttons.append(cancel);
	    buttons.append(release);*/
	    div.append(buttons);

	    this.loading.hide();


	    if ($("input[name=move]"))
	    {
	        $("input[name=move]").change(function (event)
	        {
	            var ele = event.target;
	            if ($(ele).val() == "true")
	            {
	                $("#release_new_sprint").removeAttr('disabled');
	                if ($("#release_new_sprint").val() != "")
	                    $("#release_set_current").removeAttr('disabled');
	            }
	            else
	            {
	                $("#release_new_sprint").attr('disabled', true);
	                $("#release_set_current").attr('disabled', true);
	            }
	        });
	    }

	    if ($("#release_new_sprint"))
	    {
	        $("#release_new_sprint").change(function (event)
	        {
	            var ele = event.target;
	            if ($(ele).val() == "")
	            {
	                $("#release_set_current").attr('disabled', true);
	            }
	            else
	            {
	                $("#release_set_current").removeAttr('disabled');
	            }
	        });
	    }

	    this.releaseForm.ready();

	};

	this._formWillSubmit = function(form, valid)
	{
		
		if (valid)
		{
			$("#sprintReleaseForm").hide();
			var progress = new smallProgressIndicator("releaseSprogress", "Releasing Sprint");
			$("#popupContent").append(progress.draw());
		}
		
		return true;
	};

	this._sprintReleased = function ()
	{
	    var self = this;

	    this._fetchCurrent(function (current)
	    {
	        self.tableController.currentSprint = current;
	        self.releaseDialog.vvdialog("vvclose");
	        $("#releaseDialog").empty();
	        self.selectedSprint = null;
	        self.releaseForm.reset();

	        self.tableController.refresh();
	    });
	};

	this._buildInvalidParents = function(sprints, sprint, arr)
	{
		if (sprint && sprint.recid && $.inArray(sprint.recid, arr) <= 0 )
			arr.push(sprint.recid);
		
		children = [];
		// Find child sprints
		for (var i = 0; i < sprints.length; i++)
		{
			var chksprint = sprints[i];
			
			if (chksprint.releasedate && (!sprint || sprint.parent != chksprint.recid) && $.inArray(chksprint.recid, arr) <= 0)
				arr.push(chksprint.recid);
				
			if (sprint && chksprint.parent == sprint.recid)
				arr = this._buildInvalidParents(sprints, chksprint, arr);
		}
		
		return arr;
	};
	
	// Start up the controller
	this.init();
}



function sgKickoff()
{
    checkUnload = false;
	controller = new sprintFormController();	
}
