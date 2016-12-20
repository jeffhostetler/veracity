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

function SprintTreeControl(containter, options)
{
	this.container = containter || null;
	this._data = {};
	this.milestoneTree = new vCore.milestoneTree();	

	this.options = jQuery.extend(
	{
		beforeSprintSelect: null,
		onSelect: null,
		selectCurrent: false,
		closeReleased: false

	}, (options || {}));

	this._initControl();
}

extend(SprintTreeControl, {
    selectedRecid: null,
    currentSprint: null,
    rootNode: null,

    _initControl: function (recid)
    {
        if (recid)
            this.selectedRecid = recid;

        var self = this;
        if (!this.currentSprint)
        {
            vCore.ajax({
                url: vCore.repoRoot() + "/config/current_sprint",
                dataType: 'json',
                success: function (data, status, xhr)
                {
                    self.currentSprint = data.value;
                    if (self.options.selectCurrent && self.selectedRecid == null)
                        self.selectedRecid = self.currentSprint;

                    self.draw();
                }
            });
        }
        else
        {
            if (self.options.selectCurrent && self.selectedRecid == null)
                self.selectedRecid = self.currentSprint;

            self.draw();
        }

        vCore.fetchSprints({
            includeLinked: false,
            force: true,
            success: function (data, status, xhr)
            {
                self._setSprints(data);
                if (self.selectedRecid)
                    self.selectMilestone(self.selectedRecid);
            }
        });
    },

    val: function ()
    {
        if (arguments.length == 0)
        {
            if (this.selectedRecid)
                return this.selectedRecid;
        }
        else
        {
            var recid = arguments[0];
            this.selectMilestone(recid);
        }
    },

    data: function ()
    {
        if (arguments.length == 1)
        {
            var name = arguments[0];
            return this._data[name];
        }
        else if (arguments.length == 2)
        {
            var name = arguments[0];
            var value = arguments[1];
            this._data[name] = value;
        }
    },

    getSprint: function (recid)
    {
        return this.milestoneTree.getMilestone(recid).data;
    },

    getSprints: function ()
    {
        return this.milestoneTree.getMilestones();
    },

    getSelectedSprint: function ()
    {
        var node = null
        if (this.selectedRecid)
            node = this.milestoneTree.getMilestone(this.selectedRecid);

        if (node)
            return node.data;

        return null;
    },

    _setSprints: function (data)
    {
        var self = this;
        this.milestoneTree.reset();
        $(data).each(function (i, v)
        {
            self.milestoneTree.insertMilestone(v)
        });

        this.draw();
    },

    draw: function ()
    {
        this.container.empty();
        if (this.milestoneTree.length > 0)
        {
            var toggleInactive = $('<input type="image" src="' + sgStaticLinkRoot + '/img/showmilestone.png" title="show released milestones"></input>').
	 		css({ "float": "right", "margin-right": "5px" });
            toggleInactive.toggle(
                function ()
                {
                    $(this).attr({ "src": sgStaticLinkRoot + '/img/hidemilestone.png', "title": "hide released milestones" });
                    $(".li-released").show();
                },
                function ()
                {
                    $(this).attr({ "src": sgStaticLinkRoot + '/img/showmilestone.png', "title": "show released milestones" });
                    $(".li-released").hide();
                });

            this.container.append(toggleInactive);
        }
        //        this.container.append($("<br/>"));
        var ul = $("<ul>");
        this._drawNode(ul, this.milestoneTree.root);

        this.container.addClass("sprintTreeControl");
        this.container.append(ul);

        this.bindObservers();

        if (this.selectedRecid)
            this.selectMilestone(this.selectedRecid);
    },

    _drawNode: function (cont, node)
    {
        var self = this;

        var li = $("<li>").addClass("sprint");
        if (node.data)
            li.attr("id", node.data.recid);

        var title = $("<span>").addClass("title").text(node.title);

        if (node.data && node.data.releasedate)
        {
            title.addClass("released");
            li.addClass("li-released").hide();
        }

        if (node.data && (node.data.recid == this.currentSprint))
        {
            title.addClass("current");
        }


        li.append(title);

        if (node.children.length > 0)
        {
            li.addClass("hasChildren");
            var handle = $("<a>").addClass("disclosure");
            if (node.data && node.data.releasedate && this.options.closeReleased)
                handle.addClass("closed");
            else
                handle.addClass("open");
            li.prepend(handle);

            var ul = $("<ul>");
            $.each(node.children, function (i, v)
            {
                self._drawNode(ul, v);
            });
            if (node.data && node.data.releasedate && this.options.closeReleased)
                ul.hide();
            li.append(ul);
        }

        cont.append(li);
    },

    clearSelected: function ()
    {
        var sel = $(".selected");

        $.each(sel, function (i, v)
        {
            $(v).removeClass("selected");
        });
    },

    _eleClicked: function (ele)
    {
        // Check for callback
        if (this.options.beforeSprintSelect)
        {
            // Bail if callback returns false
            if (this.options.beforeSprintSelect() === false)
                return;
        }

        this.clearSelected();
        ele.addClass("selected");

        var id = ele.attr("id");

        if (id)
        {
            this.selectedRecid = id;
        }
        else
        {
            this.selectedRecid = null;
        }

        if (this.options.onSelect)
            this.options.onSelect(id);

        $(this).trigger("change");
    },

    selectMilestone: function (recid)
    {
        if (!recid)
            return;

        var ele = $("#" + recid);
        ele.addClass("selected");

        this.selectedRecid = recid;

        if (this.options.onSelect)
            this.options.onSelect(recid);

        $(this).trigger("change");
    },

    refresh: function (recid)
    {
        //this.container.empty();
        this._initControl(recid);
    },

    bindObservers: function ()
    {
        var self = this;

        $(".sprint").each(function (i, v)
        {
            $(v).click(function (event)
            {

                event.stopPropagation();

                self._eleClicked($(this));
            });
        });

        $(".disclosure").each(function (i, v)
        {
            $(v).click(function (event)
            {
                event.stopPropagation();
                event.preventDefault();

                if ($(this).hasClass("open"))
                {
                    $(this).removeClass("open");
                    $(this).addClass("closed");
                    $("ul", $(this).parent("li")).hide();
                }
                else
                {
                    $(this).removeClass("closed");
                    $(this).addClass("open");
                    $("ul", $(this).parent("li")).show();

                }
            });

        });
    }

});
