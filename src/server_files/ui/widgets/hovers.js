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

/* 

   -add a hover control to your page and specify the fetch function (either fetchWorkItemHover or fetchChangesetHover)
    note that this should be done after the page is loaded, like in document.ready()
        var workitemhover = new hoverControl(fetchWorkItemHover);

   -hook up the hover to your control must specify control and recid (or csid)        
        workitemhover.addHover($('<li>'), recid);   

*/



function hoverInfo(recid)
{
    this.id = "";
    this.title = "";
    this.recid = recid;
    this.extraInfo = [];
    this.details = {};
    this.url = "";

}


function hoverControl(fetchFunction)
{
    this.delay = 700;
    this.timeout = null;
    this.divHasFocus = false;
    
    this.fetchFunction = fetchFunction;
    this.progress = new smallProgressIndicator("hoverLoading", "Loading summary...");
    this.div = $("<div>").css("width", "100%");
    var self = this;
    this.div.dialog({

        dialogClass: "hoverdialog primary",
        resizable: false,
        minWidth: 400,
		minHeight: 30,
						
        
        autoOpen: false
    });
  
    this.div.mouseleave(function ()
    {
        if (self.timeout)
        {
            clearTimeout(self.timeout);
        }
        self.timeout = setTimeout(function ()
        {
            self.hide();
            self.divHasFocus = false;
        }, self.delay);       
       
    });
    this.div.mouseenter(function ()
    {
        self.divHasFocus = true;       
    });
}
extend(hoverControl, {

    show: function (recid, location, baseUrl)
    {
        this.div.dialog("option", "width", 400);
        this.div.dialog("option", "position", location);
        this.div.html(this.progress.draw());
        this.div.dialog("open");

        this.fetchFunction(recid, this.div, baseUrl || null);
    },
    hide: function ()
    {
		this.div.dialog("close");
		if (self.timeout)
			clearTimeout(self.timeout);
    },
    disableHovers: function (controls)
    {
        $.each(controls, function (i, v)
        {
            $(v).unbind("mouseenter");
            $(v).unbind("mouseleave");
        });
    },
    addHover: function (control, recid, yoffset, xoffset, url)
    {
        var self = this;
        var x = xoffset || 0;
        var y = yoffset || 0;
        var baseUrl = url || null;

        if (isTouchDevice())
        {
            var img = $("<img src='" + sgStaticLinkRoot + "/img/searchgreen14.png' />").addClass("hoverToggle");
            control.append(img);
            img.toggle(
            function ()
            {
                var pos = control.offset();

                self.show(recid, [pos.left + control.width() + 5 + x, pos.top - $(window).scrollTop() + y], baseUrl);
            },
            function ()
            {
                var pos = control.offset();
                self.hide();
            });

        }
        else
        {
            control.mouseenter(function ()
            {
                if (self.timeout)
                {
                    clearTimeout(self.timeout);
                }
                self.timeout = setTimeout(function ()
                {
                    var pos = control.offset();
                    self.show(recid, [pos.left + control.width() + 5 + x, pos.top - $(window).scrollTop() + y], baseUrl);
                }, self.delay);
            });
            control.mouseleave(function ()
            {
                if (self.timeout)
                {
                    clearTimeout(self.timeout);
                }
                self.timeout = setTimeout(function ()
                {
                    if (!self.divHasFocus)
                    {
                        self.hide();
                    }
                }, self.delay);

            });
        }
    }

});

//id can be either the item id or the recid
function fetchWorkItemHover(id, div)
{
    var url = sgCurrentRepoUrl + "/workitem/" + id + "/hover.json";
    var info = new hoverInfo(id);
    vCore.ajax(
			{
			    url: url,
			    dataType: 'json',
			    success: function (data)
			    {
			        info.id = data.id;
			        info.title = vCore.htmlEntityEncode(data.title);
			        info.extraInfo = [data.status, data.milestonename || "Product Backlog", (data.priority ? data.priority + " Priority" : "(Unassigned Priority)")];
			       
			        info.url = sgCurrentRepoUrl + "/workitem.html?recid=" + data.recid;
			        info.details = { "Reported By:": data.reportername, "On:": shortDateTime(new Date(data.created_date)) };
			        if (data.assigneename)
			            info.details["Assigned To:"] = data.assigneename;
			        if (data.verifiername)
			            info.details["Verifier:"] = data.verifiername;
			        div.html(createHoverDiv(info));
					
					var ht = div.find('.hovertable');

			        if (ht.width() > 400)
			        {
			            div.dialog("option", "width", ht.width() + 20);
			        }

					div.dialog("option", "minHeight", ht.height() + 20);
			    }
			}
		);
}

function fetchChangesetHover(csid, div, baseUrl)
{
    //todo revnos in addition to csids
	baseUrl = baseUrl || sgCurrentRepoUrl;
    var url = baseUrl + "/changesets/" + csid + ".json";
    var info = new hoverInfo(csid);
    vCore.ajax(
			{
			    url: url,
			    dataType: 'json',
				noFromHeader: true,
			    success: function (data)
			    {
			        info.id = data.description.revno + ":" + data.description.changeset_id.slice(0, 10);
			        if (data.description.comments)
			        {
			            data.description.comments.sort(function (a, b)
			            {
			                return (a.history[0].audits[0].timestamp - b.history[0].audits[0].timestamp);
			            });

			            if (data.description.comments[0])
			            {
			                var vc = new vvComment(data.description.comments[0].text);

			                info.title = vc.summary;
			            }
			        }
			        info.url = baseUrl + "/changeset.html?recid=" + data.description.changeset_id;
			        var users = [];
			        var dates = [];
			        $.each(data.description.audits, function (i, v)
			        {
			            var d = new Date(parseInt(v.timestamp));
			            dates.push(shortDateTime(d));
			            var who = v.username || v.name || v.userid || ' ';
			            users.push(who);

			        });
			        info.details = { "Committed By:": users.join(", "), "On:": dates.join(" ") };
			        if (data.description.tags && data.description.tags.length)
			        {
			            info.details["tags"] = data.description.tags;
			        }
			        if (data.description.stamps && data.description.stamps.length)
			        {
			            info.details["stamps"] = data.description.stamps;
			        }

			        div.html(createHoverDiv(info));

			        if ($(".hovertable").width() > 400)
			        {
			            div.dialog("option", "width", $(".hovertable").width() + 20);
			        }
					div.dialog("option", "minHeight", $(".hovertable").height() + 20);
			    }
			}
		);
}

function createHoverDiv(info)
{
    var table = $("<table>").addClass("hovertable");
    var thead = $("<thead>");
    var trTitle = $("<tr class='dataheaderrow'></tr>");
 
    trTitle.append($("<th class='th-hoverid' rowspan='2'>" + info.id + "</th>"));
    trTitle.append($("<td>").html(info.title).addClass("th-hovertitle"));
    thead.append(trTitle);

	var extras = $.map(info.extraInfo || [], function(i) { return( vCore.htmlEntityEncode(i) ); } );
    thead.append($("<tr><td class='titlehoverextra'>" + extras.join(" | ") +
                        " <a class='a-title-hover' title='open in new tab' href='" + info.url + "' target='_blank'><img src='"  + sgStaticLinkRoot + "/img/newtab.png'/></a></td></tr>"));
    table.append(thead);

    $.each(info.details, function (i, v)
    {
        if (i == "tags")
        {
            var tr = $("<tr>").append($("<th>"));
            var td = $("<td>");
            tr.append(td);
            $.each(v, function (index, tag)
            {
                var span = $('<span>').text(tag).addClass("vvtag");
                td.append(span);
            });
            table.append(tr);
        }
        else if (i == "stamps")
        {
            var tr = $("<tr>").append($("<th>"));
            var td = $("<td>");
            tr.append(td);
            $.each(v, function (index, stamp)
            {
                var span = $('<span>').text(stamp).addClass("stamp");
                td.append(span);
            });
            table.append(tr);
        }
        else
        {
			var tr = $("<tr />").
				appendTo(table);

			tr.append( $('<th />').text(i) ).
				append($('<td />').text(v));
        }
    });
   
    return (table);
   

}
