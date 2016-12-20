/*
Copyright 2011-2013 SourceGear, LLC

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

var vvWiki = {
    oldPage: '',
    markdownLinks: new vvMarkdownLinks(),
    pageName: null,
    wipop: null,
    cspop: null,
    loadedPage: null,
    markdownEditor: null,


    enableWikiLinks: function ()
    {
        $('#wikilist li').addClass('active');
        $('.wikiadder').
			removeClass('disabled').
			removeAttr('disabled');
    },

    disableWikiLinks: function ()
    {
        $('#wikilist li').removeClass('active');
        $('.wikiadder').
			addClass('disabled').
			attr('disabled', 'disabled');
    },

    showEdits: function (e, hideCancel)
    {
        vvWiki.enableWikiLinks();

        $.each(vvWiki.edits,
			function (i, v)
			{
			    $(v).show();
			}
		);
        vvWiki.edit.hide();
        vvWiki.markdownEditor.editor.run();

        var te = vvWiki.textedit[0];
        var tl = vvWiki.textedit.val().length;
        te.selectionStart = tl;
        te.selectionEnd = tl;

        if (hideCancel)
        {
            vvWiki.cancel.hide();
            vvWiki.deletebutton.hide();
        }

        $('.wmd-panel .byline').hide();

        $('.ui-draggable').draggable('enable');
        window.editMode = true;
    },

    deletePage: function ()
    {
        var db = vvWiki.deletebutton;
        var pn = vvWiki.pageName;

        var nb = $('<strong></strong>').text('"' + pn + '"');
        var sp = $('<span>Are you sure you want to delete the </span>').
			append(nb).
			append(' page?');

        confirmDialog(
			sp, vvWiki.deletebutton,
			function (args)
			{
			    var durl = sgCurrentRepoUrl + "/wiki/page.json?title=" + encodeURIComponent(args.pageName);

			    vCore.ajax(
					{
					    'url': durl,
					    'type': 'DELETE',
					    'errorMessage': "Unable to delete this page.",
					    success: function ()
					    {
					        var d = $('<div><p>Deleted this Wiki page.</p></div>');
					        var pop = new modalPopup(
								null, null,
								{
								    onClose: function ()
								    {
								        window.onbeforeunload = null;
								        window.document.location.href = sgCurrentRepoUrl + "/wiki.html";
								    },
								    id: 'deleted'
								}
							);

					        var buttonOK = new vButton("OK").appendTo(d);

					        buttonOK.click(
								function ()
								{
								    pop.close();
								}
							);

					        $('body').prepend(pop.create('Deleted', d).addClass('info'));
					        pop.show();
					    }
					}
				);
			},
			{ "pageName": pn }
		);
    },

    listHistory: function (e)
    {
        e.stopPropagation();
        e.preventDefault();

        vvWiki.showHistory(vvWiki.hist, vvWiki.pageName);
    },

    loadHistory: function (history)
    {
        vvWiki.hist = history;
        vvWiki.history.show();
    },

    showHistory: function (history, title)
    {
        var dlg = this.historyDlg || null;
        var tbl = $('#wikihistorytable');
        var buttons = history && (history.length > 1);
        var button = null;

        if (!this.historyDlg)
        {
            this.historyDlg = $('<div></div>');
            dlg = this.historyDlg;

            var showdiff = function ()
            {
                var from = tbl.find('input[name=from]:checked').val();
                var to = tbl.find('input[name=to]:checked').val();

                var url = sgCurrentRepoUrl + "/wiki/diff.txt?" +
					"csid1=" + encodeURIComponent(from) + "&" +
					"csid2=" + encodeURIComponent(to) + "&" +
					"page=" + encodeURIComponent(title);

                var d = $('<div></div>');

                dlg.vvdialog("vvclose");
                showDiff(url, d, true, title);
            };

            var opts = {
                'position': 'center',
                'autoOpen': false,
                'title': "Page History",
                'resizable': false,
                'width': 400,
                "maxHeight": Math.floor($(window).height() * .8),
                "height": Math.floor($(window).height() * .8),
                'buttons':
				{
				    'Compare': showdiff,
				    'Cancel': function () { dlg.vvdialog("vvclose"); }
				},
                close: function (event, ui)
                {
                    $("#backgroundPopup").hide();
                }
            };

            tbl = $('<table id="wikihistorytable"></table>').
				append("<tr><th>From</th><th>To</th></tr>").
				appendTo(dlg);

            dlg.vvdialog(opts);

            var btns = dlg.vvdialog("widget").find('button');
            $.each(btns,
				function (i, v)
				{
				    if ($(v).text() == 'Compare')
				        button = $(v);
				}
			);
        }

        $('.histrow').remove();

        if (buttons)
            _enableButton(button);
        else
            _disableButton(button);

        for (var i = 0; i < history.length; ++i)
        {
            var h = history[i];

            var tr = $('<tr class="histrow"></tr>').appendTo(tbl);

            var fromtd = $('<td></td>').appendTo(tr);
            var from = $('<input type="radio" name="from" />').appendTo(fromtd).val(h.csid);
            var totd = $('<td></td>').appendTo(tr);
            var to = $('<input type="radio" name="to" />').appendTo(totd).val(h.csid);

            if (buttons)
            {
                if (i == 0)
                    to.attr('checked', 'checked');
                else if (i == 1)
                    from.attr('checked', 'checked');
            }
            else
            {
                to.attr('disabled', 'disabled').addClass('disabled');
                from.attr('disabled', 'disabled').addClass('disabled');
            }

            var td = $('<td></td>').
				text(h.audits[0].username + " / " + shortDateTime(new Date(parseInt(h.audits[0].timestamp)))).
				appendTo(tr);
        }

        tbl.find('input[type=radio]').change(
			function (e)
			{
			    var from = tbl.find('input[name=from]:checked').val();
			    var to = tbl.find('input[name=to]:checked').val();

			    if (from == to)
			        _disableButton(button);
			    else
			        _enableButton(button);
			}
		);

        dlg.vvdialog("vvopen");
    },

    grabPage: function (pageName)
    {
        pageName = pageName || 'Home';

        if (this.loadedPage && (pageName != this.loadedPage))
        {
            document.location.href = sgCurrentRepoUrl + "/wiki.html?page=" + encodeURIComponent(pageName);
            return;
        }

        this.loadedPage = pageName;

        if (pageName == 'New Page')
        {
            vvWiki.showEdits(null, true);
            return;
        }

        this.pageName = pageName;

        var purl = sgCurrentRepoUrl + "/wiki/page.json?title=" + encodeURIComponent(pageName);
        var win = window;

        var histsec = $('#wikihistory').empty();

        vCore.ajax(
			{
			    url: purl,
			    dataType: 'json',
			    reportErrors: false,
			    success: function (data)
			    {
			        $('#wmd-preview').html(vvWiki.markdownEditor.converter.makeHtml(data.text));
			        vvWiki.markdownLinks.validateLinks($('#wmd-preview'));

			        data.rectype = 'page';
			        vvWiki.form.setRecord(data);
			        vvWiki.form.ready();

			        var hist = data.history;

			        if (hist && hist.length > 0)
			            vvWiki.loadHistory(hist, histsec, data.title);

			        if (data.when)
			        {
			            var d = new Date(parseInt(data.when));

			            var p = $('<p class="byline" id="wikibyline">Updated </p>').
							append(shortDateTime(d));

			            if (data.who)
			                p.append(" by ").append(document.createTextNode(data.who));

			            p.prependTo($('#wikidetails'));
			        }
			    },
			    error: function (xhr, status, errorthrown)
			    {
			        if (xhr && (xhr.status == 404))
			        {
			            var newRec = {
			                title: pageName,
			                text: '',
			                rectype: 'page'
			            };
			            vvWiki.form.setRecord(newRec);
			            vvWiki.showEdits(null, true);
			            vvWiki.form.ready();
			        }
			        else
			        {
			            reportError("Unable to retrieve the requested page", xhr, status, errorthrown);
			        }
			    }
			}
		);
    },

    addEditor: function (roTitle)
    {
        var hideEdits = function ()
        {
            $.each(vvWiki.edits, function (i, v) { $(v).hide(); });
            vvWiki.edit.show();
            window.editMode = false;
            vvWiki.disableWikiLinks();

            $('.ui-draggable').draggable('disable');
            $('.ui-draggable').removeClass('ui-state-disabled');
        };

        var vfOptions = {
            rectype: "page",
            url: sgCurrentRepoUrl + "/wiki/page.json",
            method: "POST",
            dataType: "text",
            contentType: "application/json",
            errorFormatter: bubbleErrorFormatter,
            afterSubmit: function (vform, record)
            {
                var newPage = record.title;

                $('#wikititle').text(newPage);
                hideEdits();

                vvWiki.grabPage(vvWiki.titleedit.val());
            }
        };

        this.form = new vForm(vfOptions);

        var bb = $('#wmd-button-bar').hide();
        var editButtons = $('<div id="topbuttons" class="titlebuttons clearfix" />').appendTo($('#wikidetails'));

        this.titleedit = this.form.textField('title', { validators: { required: true, maxLength: 256, liveValidation: true} }).
			insertBefore(bb).
			hide().
			addClass('wiki').
			attr('placeholder', 'Page Title');
        this.form.errorFormatter.createContainer(this.titleedit, { "trustposition": true }).insertAfter(this.titleedit);

        if (roTitle)
            this.titleedit.
				attr('readonly', true).
				attr('disabled', true).
				addClass('disabled');

        this.textedit = this.form.textArea('text', { validators: { required: true, liveValidation: true} }).
			insertAfter(bb).
			css('display', 'block').
			hide().
			attr('id', 'wmd-input').
			addClass('wmd-input wiki innercontainer empty').
			attr('placeholder', 'Wiki Text');
        this.form.errorFormatter.createContainer(this.textedit, { "trustposition": true}).insertAfter(this.textedit);

        vvWiki.markdownEditor = new vvMarkdownEditor(this.textedit, "", $('#wikidetails'));
        vvWiki.markdownEditor.draw(bb, $('#wikipreview'));
        vvWiki.markdownEditor.prependHelpText('<p>Enter any title you choose (although it must not match the name of any other page in this wiki); that will be the name which is displayed in page lists and the activity stream, and via which other  wiki pages can link to this one.  </p> <p>You\'re free to change the page\'s name at any time.</p>');

        this.save = this.form.submitButton('Save').appendTo(editButtons).hide();
        this.cancel = this.form.button('Cancel').appendTo(editButtons).hide();
        this.rid = this.form.hiddenField('recid').insertAfter(this.save);
        this.csid = this.form.hiddenField('_csid').insertAfter(this.rid);
        var previewLabel = $('#wikipreview').hide();
        this.edits = [this.titleedit, this.textedit, this.save, this.cancel, bb, previewLabel];

        this.history = this.form.button('History').appendTo(editButtons).
			click(vvWiki.listHistory).
			hide();
        this.edit = this.form.button('Edit').appendTo(editButtons).
			click(vvWiki.showEdits);
        this.deletebutton = this.form.button('Delete').appendTo(editButtons).
			click(vvWiki.deletePage);

        if (roTitle)
            this.deletebutton.hide();

        this.cancel.click(hideEdits);
        this.cancel.click(
			function ()
			{
			    if (vvWiki.wipop)
			        vvWiki.wipop.close();
			    if (vvWiki.cspop)
			        vvWiki.cspop.close();

			    hideEdits();
			    window.editMode = false;
			    vvWiki.form.reset();
			    updateTitle();
			    vvWiki.markdownEditor.editor.refreshPreview();
			    $('.byline').show();
			}
		);


        var updateTitle = function ()
        {
            $('#wikititle').text(vvWiki.titleedit.val());
        };

        this.titleedit.change(updateTitle);
        this.titleedit.keyup(updateTitle);
    },

    insertLink: function (pageName)
    {
        var ta = $('#wmd-input');
        var before = ta.val();
        var len = before.length;

        var newLink = "[[" + pageName + "]]";

        this.markdownEditor.insertAtCaret(newLink);
        this.markdownEditor.editor.refreshPreview();
    },

	addPageLink: function(list, page, currentPageName)
	{
		var p = page.title;
		var ts = page.last_timestamp;

        if (p == currentPageName)
            return;

		var li = $('<li class="wikilistitem"></li>').
			data('ts', ts);

        $('<a></a>').
			attr('href', 'wiki.html?page=' + encodeURIComponent(p)).
			text(p).
			addClass('wikilink').
			appendTo(li);

        var addlink = $('<a></a>').
			addClass('disabled').
			addClass('wikiadder').
			prependTo(li).
			text(' ');

        $('<img />').
			attr('src', vCore.getOption('staticLinkRoot') + "/modules/wiki/trans.png").
			attr('title', "Insert a link to this page").
			prependTo(addlink);

        addlink.click(
			function ()
			{
			    if (!addlink.hasClass('disabled'))
			        vvWiki.insertLink(p + "");
			}
		);

        list.append(li);
    },

    updatePageList: function ()
    {
        var items = $('.wikilistitem');

        var searchstr = $('#wikisearch').val().toLowerCase();

        var parts = searchstr.split(/[ \t]+/) || [];

        $.each(items,
			function (i, v)
			{
			    var li = $(v);
			    var display = true;
			    var t = li.text().toLowerCase();

			    $.each(parts,
					function (j, p)
					{
					    if (t.indexOf(p) < 0)
					    {
					        display = false;
					        return false;
					    }
					}
				);

			    if (display)
			        li.show();
			    else
			        li.hide();
			}
		);
    },

    showWikiPages: function (container)
    {
        vvWiki.wil = $('<ul id="wikilist"></ul>').appendTo(container);

        var requrl = sgCurrentRepoUrl + "/wiki/pages.json?q=";
        var pn = getQueryStringVal('page') || '';

        vCore.ajax(
			{
			    "url": requrl,
			    "dataType": 'json',
			    reportErrors: false,
			    success: function (data)
			    {
			        vvWiki.wil.empty();

			        data = data || [];

			        for (var i = 0; i < data.length; ++i)
			        {
			            vvWiki.addPageLink(vvWiki.wil, data[i], pn);
			        }

					updateSort();

					if (window.editMode)
						vvWiki.enableWikiLinks();
					else
						vvWiki.disableWikiLinks();
				}
			}
		);
    }
};


function updateSort(sortby, sortdir)
{
	sortby = sortby || vCore.getCookieValueForKey('wikisort', 'by', "date");
	sortdir = sortdir || vCore.getCookieValueForKey('wikisort', 'dir', "desc");

	vCore.setCookieValueForKey('wikisort', 'by', sortby, 30);
	vCore.setCookieValueForKey('wikisort', 'dir', sortdir, 30);

	var sel = $('#sortbydate');
	var unsel = $('#sortbyname');

	if (sortby == "name")
	{
		sel = $('#sortbyname');
		unsel = $('#sortbydate');
	}

	unsel.removeClass('sortby');

	sel.
		addClass('sortby').
		removeClass('asc desc').
		addClass(sortdir);

	var ascend = (sortdir == 'asc');

	var sortByDate = function(a,b) {
		var lia = $(a);
		var lib = $(b);

		if (ascend)
			return( lia.data('ts') - lib.data('ts') );
		else
			return( lib.data('ts') - lia.data('ts') );
	};

	var sortByName = function(a,b) {
		var lia = $(a);
		var lib = $(b);

		if (ascend)
			return( lia.text().localeCompare(lib.text()) );
		else
			return( lib.text().localeCompare(lia.text()) );
	};

	var ul = $('#wikilist');

	var items = ul.find('li');

	if (sortby == 'date')
		items.sort(sortByDate)
	else
		items.sort(sortByName);

	$.each(items,
		function(i,li) {
			ul.append(li);
		}
	);
}

function addSorters(ctr)
{
	ctr.append(' ');
	var bydate = $('<a href="#" class="sorttoggle" id="sortbydate">date</a>').
		appendTo(ctr);
	ctr.append(' ');
	var byname = $('<a href="#" class="sorttoggle" id="sortbyname">name</a>').
		appendTo(ctr);

	updateSort();

	function clicker(el, val) {
		if (el.hasClass('sortby'))
		{
			if (el.hasClass('desc'))
				updateSort(val, 'asc');
			else
				updateSort(val, 'desc');
		}
		else
		{
			if (el.hasClass('desc'))
				updateSort(val, 'desc');
			else
				updateSort(val, 'asc');
		}
	}

	bydate.click(
		function() {
			clicker(bydate, 'date');
			return false;
		}
	);
	byname.click(
		function() {
			clicker(byname, 'name');
			return false;
		}
	);
}

$(document).ready(
	function ()
	{
	    window.onbeforeunload = null;
	    window.onbeforeunload = function ()
	    {
	        if (window.editMode && vvWiki.form.isDirty())
	            return "You have unsaved changes.";
	    }

	    var pn = getQueryStringVal('page') || 'Home';

	    vvWiki.addEditor(pn == 'Home');

	    document.title = (pn || "wiki") + " (" + startRepo + ")";
	    vvWiki.oldPage = pn;
	    $('#wikititle').text(pn);

	    var pu = function () { vvWiki.updatePageList(); };

	    var rsb = $('#rightSidebar');

	    $("<h1 class='outerheading'>Wiki Pages</h1>").appendTo(rsb);
	    var nav = $('<div class="secondarynav"></div>').
			appendTo(rsb);
	    var ws = $("<input id='wikisearch' class='searchinput' />'").
			appendTo(nav).
			keyup(pu).
			change(pu);

		var sorts = $('<div class="sorter" />').
			append("Sort by").
			appendTo(rsb);

		addSorters(sorts);

		var holder = $('<div class="outercontainer"></div>').
			css('overflow', 'auto').
			appendTo(rsb);

	    vvWiki.showWikiPages(holder);

	    vCore.showSidebar();
	    vCore.resizeMainContent();

	    $("#wikihelpclose").click(function (e)
	    {
	        e.preventDefault();
	        e.stopPropagation();

	        $("#wikihelp").hide();

	    });

	    vvWiki.grabPage(pn);
	}
);
