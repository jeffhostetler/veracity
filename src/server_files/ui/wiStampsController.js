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

var stampsController = function (parentForm, afterAddFunction)
{
    this.parentForm = parentForm;
    this.stamps = [];
    this.searchForm = null;
    this.forms = [];
    this.stampContainer = null;
    this._selected = false;
    this.errorFormatter = new bubbleErrorFormatter();
    this.searchfield = null;
    this.showAdd = true;
    this.afterAddFunction = afterAddFunction || null;
}

$.widget("custom.stampcomplete", $.ui.autocomplete, {
	
	redraw: function()
	{
		var ul = this.menu.element;
		
		ul.position( $.extend({
			of: this.element
		}, this.options.position ));
	}
});


extend(stampsController, {

    setStamps: function (stamps)
    {
        this.stamps = stamps || [];
        $("#wiStampCont").empty();
        this.forms = [];
        this.renderStamps();
    },

    render: function ()
    {
        var self = this;

        var stampDiv = $("<div>").attr("id", "wiStampsWrapper").css('overflow', 'hidden').addClass('clearfix');

        this.stampContainer = $("<div />").attr("id", "wiStampCont").addClass('innercontainer blank nonedit').html('&#160;');
        stampDiv.append(this.stampContainer);
        var stampSearchDiv = $("<div>").attr("id", "wiStampSearch").css('width', '100%');

        var search = $("<input type='text' class='searchinput formel' maxlength='255' name='search'/>").attr('id', "wiSearchInput");
        search.stampcomplete({
            minLength: 2,
            source: function (request, response)
            {
                self.errorFormatter.hideInputError(self.searchField);
                return self.fetchStamps(request, response);
            },
            select: function (event, ui) { self.stampSelected(event, ui); },
            close: function (event, ui)
            {
                if (self.selected)
                {
                    $("#wiSearchInput").val("");
                    $("#wiSearchInput").focus();
                }
                self.searchField.field.removeClass("ui-autocomplete-loading");
                self.selected = false;
            },
            position: {
                my: "right top",
                at: "right bottom",
                collision: "none"
            },
            appendTo: stampDiv
        });
        search.bind("keyup", function (event)
        {
            if (event.keyCode == 13)
            {
                self.addStamp($(event.target).val());
                $("#wiSearchInput").val("");
                search.autocomplete("close");
            }
            else if (event.keyCode == 27)
                $("#wiSearchInput").val("");
        });
        search.focus(function (event)
        {
            if ($("#wiSearchInput").val() == "")
                return;

            search.autocomplete("search");
        });

        $(window).resize(function ()
        {
            search.stampcomplete("redraw");
        });

        var searchError = this.errorFormatter.createContainer(search)
        this.searchField = new vFormField(null, null, search, null, searchError);



        stampSearchDiv.append(search);
        stampSearchDiv.append(searchError);
        if (this.showAdd)
        {
            var ctr = $('<div class="buttoncontainer"></div>').appendTo(stampSearchDiv);

            var add = $("<input type='button' class='stampadder clearfix' name='Add' value='Add'/>").
				attr('id', "wiSearchAdd").
				bind("click", function () { self.addStamp(search.val()); }).
				appendTo(ctr);

            _enableOnInput(add, this.searchField.field);
        }

        stampDiv.append(stampSearchDiv);

        return stampDiv;
    },

    renderStamps: function ()
    {
        var self = this;
        var any = false;
        $.each(this.stamps, function (i, stamp)
        {
            stamp.rectype = "stamp";
            var ele = self.stampElement(stamp);

            self.stampContainer.append(ele);
            any = true;
        });

        if (!any)
            self.stampContainer.append('&#160;');
    },

    fetchStamps: function (request, response)
    {
        var self = this;
        var err = this.validateStamp(request.term);
        if (err)
        {
            err = ["Stamps " + err];

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
    },

    addStamp: function (stamp)
    {
        stamp = stamp.trim();

        if (!stamp)
            return;

        var error = this.validateStamp(stamp);

        if (error)
        {
            err = ["Stamps " + error];

            this.errorFormatter.displayInputError(this.searchField, err);
            return;
        }

        found = false;
        $.each(this.forms, function (i, form)
        {
            var rec = form.getFieldValues();
            if (rec.name == stamp)
            {
                found = true;
                var delField = form.getField("_delete");
                delField.setVal("", false);
            }
        });

        if (found)
        {
            ele = $("#stamp_" + stamp);
            $(".delete", ele).show();
            ele.removeClass("deleted");

            $("#wiSearchInput").val("");
            return;
        }
        var rec = {
            rectype: "stamp",
            name: stamp
        }

        var ele = this.stampElement(rec);

        this.stampContainer.append(ele);

        $("#wiSearchInput").val("");
        if (this.afterAddFunction)
            this.afterAddFunction();
    },

    validateStamp: function (stamp)
    {
        // var noInvalidChars = /^[^,\\\/\r\n]*$/;
        var error = validate_object_name(stamp, get_object_name_constraints(CONSTRAINT_TYPE_WIT_STAMP));
        return error;
    },

    stampElement: function (stamp)
    {

        var self = this;
        var opts = {
            rectype: "stamp",
            formKey: "stamps",
            multiRecord: true,
            prettyReader: true
        }

        if (!stamp.recid)
            opts.forceDirty = true;

        var form = new vForm(opts);
        form.setRecord(stamp);

        var cont = $("<div>").addClass("cont").attr("id", "stamp_" + stamp.name);

        var nameCell = $("<span>").addClass("stampNameCell");

        form.field(nameCell, "hidden", "_delete", { reader: { hasReader: false} });
        if (stamp.recid)
            form.field(nameCell, "hidden", "recid", { reader: { hasReader: false} });

        var name = form.field(nameCell, "text", "name", {
            reader: {
                addClass: "stamp remoteLink",
                tag: "span",
                formatter: function (val)
                {
                    var url = vCore.repoRoot() + "/workitems.html?stamp=" + vCore.urlEncode(val) + "&sort=priority&groupedby=status&columns=id%2Ctitle%2Cassignee%2Creporter%2Cpriority%2Cmilestone%2Cstamp&maxrows=100&skip=0";
                    var _val = val.length > 20 ? val.substr(0, 20) + "..." : val;
                    var link = $("<a>").attr({ "href": url, "title": val }).text(_val);
                    return link;
                }
            }
        });

        cont.append(nameCell);

        var deleteLink = $("<a>").addClass("delete").text("x");
        deleteLink.click(function () { self.deleteClicked(form, cont); });
        cont.append(deleteLink);

        form.ready();
        form.disable();
        this.forms.push(form);
        this.parentForm.addSubForm(form);

        return cont;
    },

    deleteClicked: function (form, ele)
    {
        var delField = form.getField("_delete");
        delField.setVal("true", false);

        $(".delete", ele).hide();
        ele.addClass("deleted");
    },

    disable: function ()
    {
        $("#wiStampSearch").hide();
        $(".cont .delete").hide();

        if (this.stamps.length == 0 && this.forms.length == 0)
        {
            $("#stamps").hide();
        }
    },

    enable: function ()
    {
        $("#wiStampSearch").show();
        $(".cont .delete").show();
        if (this.stamps.length == 0 && this.forms.length == 0)
        {
            $("#stamps").show();
        }
    },

    reset: function ()
    {
        $("#wiStampCont").empty();
        for (var i = 0; i < this.forms.length; i++)
        {
            this.parentForm.removeSubForm(this.forms[i]);
        }
        this.forms = [];
        this.renderStamps();
    },
	
	isDirty: function()
	{
		vCore.consoleLog("stamp is dirty called");
		if ($("#wiSearchInput").val() != "")
			return true;
		
		return false;
	}
});
