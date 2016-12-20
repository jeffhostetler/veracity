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


function vvMarkdownLinks()
{
    this.changesetHover = null;
    this.workitemHover = null;
    this.knownGoodLinks = {};
    this.knownBadLinks = {};
    this.knownGoodPages = {};
    this.knownBadPages = {};
    this._init();
}
extend(vvMarkdownLinks, {
    _init: function ()
    {
        this.changesetHover = new hoverControl(fetchChangesetHover);
        this.workitemHover = new hoverControl(fetchWorkItemHover);
    },
    setGoodLink: function (ln, href, status)
    {
        this.knownGoodLinks[href] = true;
        ln.addClass('validWikiUrl');
        ln.removeClass('invalidWikiUrl');

        var matches = href.match(/^(.+)\/changeset\.html\?recid=([a-g0-9]+)/);

        if (matches)
        {
            var csid = matches[2];
            var repUrl = matches[1];
            this.changesetHover.addHover(ln, csid, null, null, repUrl);
            return;
        }

        matches = href.match(/\/wiki\/workitem\/([a-z0-9]+)/i);

        if (matches)
        {
            var recid = matches[1];
            this.workitemHover.addHover(ln, recid);

            if (status && (status != "open"))
                ln.addClass('fixedbug');
            if (status && (status == 'verified'))
                ln.addClass('verifiedbug');
        }
    },
    setBadLink: function (ln, href, purl)
    {
        this.knownBadLinks[href] = true;
        ln.addClass('invalidWikiUrl');
        ln.removeClass('validWikiUrl');

        this.changesetHover.disableHovers([ln]);
        this.workitemHover.disableHovers([ln]);
    },
    setGoodPage: function (ln, pageName)
    {
        this.knownGoodPages[pageName] = true;
        ln.addClass('validWikiUrl');
        ln.removeClass('invalidWikiUrl');
        ln.attr('title', "Wiki: \"" + pageName + "\"");
    },

    setBadPage: function (ln, pageName)
    {
        this.knownBadPages[pageName] = true;
        ln.addClass('invalidWikiUrl');
        ln.removeClass('validWikiUrl');
        ln.attr('title', "No wiki page defined for \"" + pageName + "\"");
    },
    validateLink: function (ln)
    {
        var self = this;
        var href = ln.attr('href') || '';

        var re = new RegExp(sgCurrentRepoUrl + "/wiki.html\\?page=([^\\?&]+)");

        var matches = href.match(re);
        var win = window;

        if (matches)
        {
            var pageName = decodeURIComponent(matches[1]);
            if (pageName != "New Page")
            {

                if (this.knownGoodPages[pageName])
                    this.setGoodPage(ln, pageName);
                else if (this.knownBadPages[pageName])
                    this.setBadPage(ln, pageName);
                else
                {
                    var purl = sgCurrentRepoUrl + "/wiki/page.json?title=" + encodeURIComponent(pageName);

                    vCore.ajax(
					{
					    'type': 'HEAD',
					    url: purl,
					    dataType: 'json',
					    reportErrors: false,
					    success: function (data)
					    {
					        self.setGoodPage(ln, pageName);
					    },
					    error: function ()
					    {
					        self.setBadPage(ln, pageName);
					    }
					});
                }
            }

            return;
        }

        re = new RegExp(sgCurrentRepoUrl + "/wiki/workitem/([a-zA-Z]+[0-9]+)");

        matches = href.match(re);

        if (matches)
        {
            if (this.knownGoodLinks[href])
                this.setGoodLink(ln, href);
            else if (this.knownBadLinks[href])
                this.setBadLink(ln, href);
            else
            {
                var purl = href.replace(/workitem/, "workitem-status");

                vCore.ajax(
					{
					    url: purl,
					    dataType: 'text',
					    reportErrors: false,
					    success: function (data, textStatus, xhr)
					    {
					        self.setGoodLink(ln, href, data);
					    },
					    error: function ()
					    {
					        self.setBadLink(ln, href);
					    }
					});
            }

            return;
        }

        re = new RegExp("^(.+)/changeset\\.html\\?recid=([0-9a-g]+)");

        matches = href.match(re);

        if (matches)
        {
            var csid = matches[2];
            var repurl = matches[1];

            if (this.knownGoodLinks[href])
                this.setGoodLink(ln, href);
            else if (this.knownBadLinks[href])
                this.setBadLink(ln, href);
            else
            {
                var purl = repurl + "/changesets/" + csid + ".json";

                vCore.ajax(
					{
					    url: purl,
					    dataType: 'json',
					    reportErrors: false,
					    noFromHeader: true,
					    success: function (data)
					    {
					        self.setGoodLink(ln, href);
					    },
					    error: function (a, b, c)
					    {
					        self.setBadLink(ln, href, purl);
					    }
					});
            }

            return;
        }
    },
    validateLinks: function (cont)
    {
        var self = this;
        var container = cont || $('#wmd-preview' + this.postfix);
        var lns = container.find('a');

        $.each(lns, function (i, v) { self.validateLink($(v)); });
    }

});

function vvMarkdownEditor(textArea, postfix, container, addwrapper)
{
    this.markdownLinks = null;
    this.wipop = null;
    this.cspop = null;
    this.editor = null;
    this.convertor = null;
    this.textedit = textArea;
    this.postfix = postfix || "";
    this.filterpop = null;
    this.container = container;
    this.searchinp = null;
    this.cssearchinp = null;
    var self = this;
    this.helpdialog = null;
    this.trange = null;
    this.addwrapper = addwrapper || false; 
    this.divButtons = null;
    this.editorName = "";
    this.witCharStart = false;
    this.csCharStart = false;
	this.wikipop = null;
    this._init();
}

extend(vvMarkdownEditor, {

    _init: function ()
    {
        if (this.textedit)
        {
            this.editorname = this.textedit.attr("name");
        }
        this.markdownLinks = new vvMarkdownLinks();
        var wikiUrl = sgCurrentRepoUrl + "/wiki.html";

        this.converter = Markdown.getSanitizingConverter();
        
        this.converter.hooks.chain
		(
			"preConversion", function (text)
			{
			    text = text || "";

			    // can't trust IE 8 to split properly, so we do it ourselves.
			    // var lines = text.split(/\r?\n/);

			    var lines = [];

			    var remainder = text;
			    var pat = /\r?\n/;
			    var matches;

			    while (matches = pat.exec(remainder))
			    {
			        var mp = matches.index;
			        var rest = mp + matches[0].length;
			        lines.push(remainder.slice(0, mp));
			        remainder = remainder.slice(rest);
			    }

			    if (remainder)
			        lines.push(remainder);

			    if (lines.length == 0)
			        lines.push("");

			    var lastBlank = true;
			    var inCode = false;

			    var code = /^    /;

			    for (var i = 0; i < lines.length; ++i)
			    {
			        var line = lines[i];

			        inCode = (lastBlank || inCode) && line.match(code);

			        if (!inCode)
			        {
			            line = vCore.wikiLinks(line, wikiUrl);
			            line = vCore.bugLinks(line);
			            line = vCore.csLinks(line);
			            line = vCore.filterLinks(line);
                        line = vCore.branchLinks(line);
                        line = vCore.tagLinks(line);

			            lines[i] = line;
			        }

			        lastBlank = !line.length;
			    }

			    return (lines.join("\n"));
			}
		);

    },

    draw: function (buttons, preview)
    {
        var self = this;
        if (!buttons)
        {
            this.divButtons = $('<div id="wmd-button-bar' + this.postfix + '"></div>').addClass("markdownbuttonwrap markdowneditor");
            this.container.prepend(this.divButtons);
        }
        else
            this.divButtons = buttons;

        if (!preview)
        {
            this.container.append($('<div class="innerheading neveredit markdowneditor" style="font-weight:normal; text-transform:capitalize;">' + this.editorName + ' Preview</div>'));
            this.container.append($('<div id="wmd-preview' + this.postfix + '" class="wiki innercontainer empty markdowneditor"></div>'));
        }

        if (this.addwrapper)
        {
            $('<div id="div-markdown-label" class="markdowneditor"><span class="innerheading markdownlabel" style="float:none !important;text-transform:capitalize;">' + this.editorName + '<span></div>').insertBefore(this.divButtons);
        }
        var wikiUrl = sgCurrentRepoUrl + "/wiki.html";
        this.wipop = $("<div>").addClass("primary ui-dialog-nobuttons")
             .dialog({
                 "title": "Insert a Work Item Link",
                 "resizable": false,
                 "width": 200,
                 "height": 75,
                 "modal": false,
                 autoOpen: false,
                 open: function ()
                 {
                     self.searchinp.val('');
                 },
                 close: function (event, ui)
                 {
                     self.searchinp.autocomplete('close');
                     //if the user presses esc key or the x button on the dialog add a literal ^ 
                     if (self.witCharStart && (($(event.target) && $(event.target).hasClass("ui-icon-closethick")) || (event.originalEvent && event.originalEvent.keyCode == '27')))
                     {
                         self.insertAtCaret("\\^");
                         self.editor.refreshPreview();
                     }

                     vvWiki.textedit.focus();
                     self.textedit.focus();
                 }
             });
        this.helpdialog = $("<div>").addClass("primary ui-dialog-nobuttons")
            .dialog({
                "title": "Editor Help",
                "resizable": false,
                "modal": false,
                "width": 600,
                autoOpen: false,
                buttons: {
                    "Close": function ()
                    { $(this).dialog("close"); }
                },
                close: function (event, ui)
                {
                    self.textedit.focus();
                }
            });
        this._setHelpText();
        this.cspop = $("<div>").attr("id", "cspopup").addClass("primary ui-dialog-nobuttons")
            .dialog({
                "title": "Insert a Commit Link",
                "resizable": false,
                "width": 400,
                "height": 75,
                "modal": false,
                autoOpen: false,
                open: function ()
                {
                    self.cssearchinp.val('');
                },
                close: function (event, ui)
                {
                    self.cssearchinp.autocomplete('close');
                    //if the user presses esc key or the x button on the dialog add a literal @
                    if (self.csCharStart && (($(event.target) && $(event.target).hasClass("ui-icon-closethick")) || (event.originalEvent && event.originalEvent.keyCode == '27')))
                    {
                        self.insertAtCaret("\\@");
                        self.editor.refreshPreview();

                    }
                    self.textedit.focus();
                }
            });

        this.filterpop = $("<div>").attr("id", "filterpopup").addClass("primary ui-dialog-nobuttons")
            .dialog({
                "title": "Insert a Work Item Filter Link",
                "resizable": false,
                "width": 400,
                "height": 75,
                "modal": false,
                autoOpen: false,
                open: function ()
                {
                    self.filtersearchinp.val('');
                },
                close: function (event, ui)
                {
                    self.textedit.focus();
                }
            });
			
        this.wikipop = $("<div>").attr("id", "wikipopup").addClass("primary ui-dialog-nobuttons")
            .dialog({
                "title": "Insert Wiki Link",
                "resizable": false,
                "width": 400,
                "height": 75,
                "modal": false,
                autoOpen: false,
                open: function ()
                {
                    self.wikisearchinp.val('');
                },
                close: function (event, ui)
                {
                    self.textedit.focus();
                }
            });
				
        this._hookUpTextEvents();
        this._setupWitSearchInput();
        this._setupCsSearchInput();
        this._setupFilterSearchInput();     
		this._setupWikiSearchInput();
		
        this.editor = new Markdown.Editor(this.converter, this.postfix,
        {
            'handler': function ()
            {
                self.helpdialog.dialog("open");
            }
        });
        this.editor.hooks.chain("onPreviewRefresh", function ()
        {
            self.markdownLinks.validateLinks($('#wmd-preview' + self.postfix));
        });

        this.editor.setVvMarkdown(this);
        this.editor.hooks.set("insertImageDialog", self.imagePopup);
        this.editor.run();

    },
    prependHelpText: function (text)
    {
        this.helpdialog.prepend($(text));
    },
    _setHelpText: function ()
    {
        var help = "<p>Veracity uses <a href=\"http://daringfireball.net/projects/markdown/syntax\" class=\"helplink\" title=\"Markdown Syntax and Documentation\" tabindex='-1'>Markdown</a> to convert plain text to HTML.  You can either enter Markdown directly, or use the formatting toolbar.</p>" +
        "<p>Veracity's Markdown dialect uses double-square-brackets to link to Wiki pages by name.  e.g., to link to a page named \"Release Notes 2\", you would enter:</p>" +
        "<pre><code>[[Release Notes 2]]</code></pre> " +
        "<p>You can link to Veracity work items using the <code>^</code> character, and to changesets using the <code>@</code> character, e.g.:</p>  " +
        "<pre><code>^A0026</code>\n" +
        "<code>@g44f123123aaaa</code></pre>" +
        "<p>You can also link to <em>symbolic</em> changeset names &mdash; the heads of branches, or version control tags.  In those cases, use standard Markdown links with <code>branch://...</code> or <code>tag://...</code> URLs.  Use double quotes around tag or branch names containing spaces.</p>" +
        "<pre><code>[the master branch](branch://master)</code>\n" +
        "<code>[Release 1.0](tag://release_1_0)</code>\n" +
        "<code>[Beta Release](tag://\"beta test\")</code></pre>";

        this.helpdialog.html($(help));
    },

    insertAtCaret: function (myValue)
    {
        var e = this.textedit[0];

        if (e.selectionStart || e.selectionStart == '0')
        {
            var startPos = e.selectionStart;
            var endPos = e.selectionEnd;
            var scrollTop = e.scrollTop;
            e.value = e.value.substring(0, startPos) + myValue + e.value.substring(endPos, e.value.length);
            e.focus();
            e.selectionStart = startPos + myValue.length;
            e.selectionEnd = startPos + myValue.length;
            e.scrollTop = scrollTop;
        }
        else if (this.trange)
        {
            e.focus();
            this.trange.text = myValue;
            e.focus();
            this.trange.select();
            this.trange = false;
        }
        else
        {
            e.value += myValue;
            e.focus();
        }
    },
    _loadZero: function (input)
    {
        if (input.data('fired') || !input.data('waiting'))
            return;

        if (input.val())
            return;

        if (!input.is(':visible'))
            return;

        input.autocomplete('search', '');

    },
    _setupCsSearchInput: function ()
    {
        var self = this;
        var csadiv = $('<div></div>');
        this.cssearchinp = $('<input />');
        csadiv.append(this.cssearchinp);
        this.cspop.html(csadiv);

        var label = $('<label for="csrepo"> in repo </label>').
			appendTo(csadiv);
        var pullDown = $('<select id="csrepo"></select>').
			appendTo(csadiv);

        vCore.fetchRepos(
			{
			    success: function (data)
			    {
			        $.each(
						data,
						function (i, descriptor)
						{
						    var entry = $('<option></option>').
								text(descriptor).
								val(descriptor).
								appendTo(pullDown);
						    if (descriptor == startRepo)
						        pullDown.val(descriptor);
						}
					);
			    }
			}
		);


        this.cssearchinp.blur(function ()
        {
            self.cssearchinp.data('waiting', false);
        });

        this.cssearchinp.focus(function (e)
        {
            self.cssearchinp.data('fired', false);
            var v = self.cssearchinp.val();

            if (!v)
            {
                self.cssearchinp.data('waiting', true);
                window.setTimeout(function () { self._loadZero(self.cssearchinp) }, 3000);
            }
        });

        this.cssearchinp.autocomplete(
		{
		    source: function (req, callback)
		    {
		        self.cssearchinp.autocomplete('close');
		        self.cssearchinp.data('fired', true);

		        var term = req.term;
		        var results = [];

		        var isNumber = term.match(/^[0-9]+$/);

		        if (term && (term.length < 3) && (!isNumber))
		        {
		            callback([]);
		            return;
		        }

		        var reponame = $('#csrepo').val() || '';
		        var repurl = sgCurrentRepoUrl;

		        if (reponame)
		            repurl = sgCurrentRepoUrl.replace("repos/" + encodeURIComponent(startRepo), "repos/" + encodeURIComponent(reponame));

		        var u;
		        var last10 = false;
		        var user = null;
		        var fireatwill = false;

		        if (term)
		        {
		            u = repurl + "/wicommitsearch.json?q=" + encodeURIComponent(term);

		            if (user)
		                u += "&userid=" + encodeURIComponent(user);
		        }
		        else
		        {
		            u = repurl + "/history.json?max=10";
		            if (user)
		            {
		                user = userList.find('option:selected').text();
		                u += "&user=" + encodeURIComponent(user);
		            }

		            last10 = true;
		        }

		        vCore.ajax(
					{
					    url: u,
					    dataType: 'json',
					    noFromHeader: true,
					    errorMessage: 'Unable to retrieve commits at this time.',

					    success: function (data)
					    {
					        results = [];

					        if (last10)
					        {
					            var items = data.items || [];

					            for (var i = 0; i < items.length; ++i)
					            {
					                var item = items[i];
					                var rec = {};
					                var text = '';

					                if (item.comments && (item.comments.length > 0))
					                    text = item.comments[0].text.substring(0, 140);
					                rec.label = item.revno + ":" + item.changeset_id.substring(0, 10) + "..." +
										"\t" + text;
					                rec.text = text;
					                rec.value = item.changeset_id;
					                rec.csid = item.changeset_id;

					                results.push(rec);
					            }
					        }
					        else
					        {
					            if (fireatwill && isNumber && (data.length == 1) && !already[data[0].value])
					            {
					                addSelected(data[0], false);
					                results = [];
					                csrelsearch.val('');
					            }
					            else
					                for (var i = 0; i < data.length; ++i)
					                    results.push(data[i]);
					        }
					    },

					    complete: function ()
					    {
					        if (self.cssearchinp.is(':visible'))
					            callback(results);
					    }
					}
				);
		    },
		    delay: 400,
		    minLength: 0,
		    select: function (event, ui)
		    {
		        event.stopPropagation();
		        event.preventDefault();

		        self.cssearchinp.autocomplete('close');

		        var reponame = $('#csrepo').val() || '';

		        var v = ui.item.value;

		        if (reponame && (reponame != startRepo))
		            v = reponame + ":" + v;

		        self.insertAtCaret("@" + v);
		        self.editor.refreshPreview();

		        self.cspop.dialog("close");

		        return (false);
		    }
		});

        var csel = this.cssearchinp.data('autocomplete').element;
        csel.oldVal = csel.val;
        csel.val = function (v)
        {
            return (csel.oldVal());
        };

    },
    _setupWitSearchInput: function ()
    {
        var self = this;
        var adiv = $("<div>").attr("id", "searchInputDialog");

        this.searchinp = $('<input />');
        adiv.append(this.searchinp);
        this.wipop.html(adiv);
        this.searchinp.blur(function ()
        {
            self.searchinp.data('waiting', false);
        });

        this.searchinp.focus(function (e)
        {
            self.searchinp.data('fired', false);
            var v = self.searchinp.val();

            if (!v)
            {
                self.searchinp.data('waiting', true);
                window.setTimeout(function () { self._loadZero(self.searchinp) }, 3000);
            }
        });

        this.searchinp.autocomplete(
        {
            'source': function (req, callback)
            {
                self.searchinp.autocomplete('close');
                self.searchinp.data('fired', true);

                var term = req.term;
                var results = [];

                var u = sgCurrentRepoUrl + "/workitem-fts.json?term=" + encodeURIComponent(term || '');

                if (!term)
                {
                    var vlist = _visitedItems() || [];

                    var ids = [];
                    $.each(vlist, function (i, v)
                    {
                        var nv = v.split(":");
                        ids.push(nv[0]);
                    });

                    u = sgCurrentRepoUrl + "/workitem-details.json?_ids=" + encodeURIComponent(ids.join(','));
                }

                vCore.ajax({
                    url: u,
                    dataType: 'json',
                    errorMessage: 'Unable to retrieve work items at this time.',

                    success: function (data)
                    {
                        results = [];

                        for (var i = 0; i < data.length; ++i)
                            results.push(data[i]);
                    },

                    complete: function ()
                    {
                        if (self.searchinp.is(':visible'))
                            callback(results);
                    }
                });
            },
            'delay': 400,
            'minLength': 0,
            'select': function (event, ui)
            {
                event.stopPropagation();
                event.preventDefault();
                self.searchinp.autocomplete('close');

                var matches = ui.item.label.match(/^([A-Z]+[0-9]+)/i);

                if (matches)
                {
                    self.insertAtCaret("^" + matches[1]);
                    self.editor.refreshPreview();
                }

                self.wipop.dialog("close");

                return (false);
            }
        });

    },
    _setupFilterSearchInput: function ()
    {
        var self = this;
        var adiv = $("<div>").attr("id", "filtersearchInputDialog");

        this.filtersearchinp = $('<select />').
            appendTo(adiv).
            append($('<option value="">Select A Filter</option>')).
            append($('<option value="default">My Current Items</option>'));

        this.filterpop.html(adiv);

        vCore.ajax(
        {
            'url': sgCurrentRepoUrl + "/filters.json?details",
            'dataType': 'json',
            'success': function(data) {
                $.each(data,
                    function(i,flt) {
                        $('<option></   option>').
                        val(flt.recid).
                        text(flt.name).
                        appendTo(self.filtersearchinp);
                    }
                );
            }
        });

        // magically fill from     var url = sgCurrentRepoUrl + "/filters.json?details";

        this.filtersearchinp.change(
            function( event ) {
                event.stopPropagation();
                event.preventDefault();

            var val = self.filtersearchinp.val();

            if (val)
            {
                var name = self.filtersearchinp.find('option:selected').text();
                
                self.insertAtCaret("[" + name + "](filter://" + val + ")");
                self.editor.refreshPreview();
            }

            self.filterpop.dialog("close");

            return (false);
        });
    },
	_setupWikiSearchInput: function()
	{
        var self = this;
        var adiv = $("<div>").attr("id", "wikiSearchInputDialog");
		
		this.wikisearchinp = $('<input />');
		
        adiv.append(this.wikisearchinp);
        this.wikipop.html(adiv);
        this.wikisearchinp.blur(function ()
        {
            self.searchinp.data('waiting', false);
        });
		
        this.wikisearchinp.focus(function (e)
        {
            self.wikisearchinp.data('fired', false);
            var v = self.wikisearchinp.val();

            if (!v)
            {
                self.wikisearchinp.data('waiting', true);
                window.setTimeout(function () { self._loadZero(self.wikisearchinp) }, 3000);
            }
        });
		
        this.wikisearchinp.autocomplete(
        {
			'source': function(req, callback) {
                self.wikisearchinp.autocomplete('close');
                self.wikisearchinp.data('fired', true);

                var term = req.term;
                var results = [];
				
                var u = sgCurrentRepoUrl + "/wiki/pages.json";
				
                vCore.ajax({
                    url: u,
                    dataType: 'json',
                    errorMessage: 'Unable to retrieve wiki pages at this time.',

                    success: function (data)
                    {
                        results = [];

                        for (var i = 0; i < data.length; ++i)
						{
							if (data[i].title.indexOf(term) >= 0)
								results.push(data[i].title);
						}
                            
                    },

                    complete: function ()
                    {
                        if (self.wikisearchinp.is(':visible'))
                            callback(results);
                    }
                });
				
			},
            'delay': 400,
            'minLength': 0,
            'select': function (event, ui)
            {
                event.stopPropagation();
                event.preventDefault();
                self.wikisearchinp.autocomplete('close');
				
				page = ui.item.label
				
                self.insertAtCaret("[[" + page + "]]");
                self.editor.refreshPreview();
                self.wikipop.dialog("close");

                return (false);
            }
		});
		
	},
    imagePopup: function (callback)
    {
        var cb = callback;
        $("#imgup").remove();
        var adiv = $("<div>").attr("id", "imgup").addClass("primary ui-dialog-nobuttons");

        //  var adiv = $('<div></div>');
        var uploader = $('<input type="file" id="attachfile" name="attachfile" />').
			appendTo(adiv);

        var go = $(document.createElement('input')).
			attr('type', 'button').
			attr('id', 'attachbutton').
			css('float', 'right').
			val('Attach').
			appendTo(adiv);

        var uploadurl = sgCurrentRepoUrl + "/wiki/image?From=" + encodeURIComponent(sgUserName);
        var cbData = null;

        go.click(
			function ()
			{
			    if (!uploader.val())
			        return;

			    cb = null;

			    $.ajaxFileUpload(
					{
					    "url": uploadurl,
					    "dataType": "json",
					    "fileElementId": 'attachfile',
					    "secureuri": "false",
					    "opener": window,
					    "pop": adiv,

					    complete: function ()
					    {
					        callback(cbData);
					    },

					    success: function (data, textStatus)
					    {
					        this.pop.vvdialog("vvclose");

					        if (!data)
					        {
					            this.opener.reportError("Unable to update at this time. Your attachment was not saved.", false);
					            return;
					        }
					        else if (typeof (data.error) != 'undefined')
					        {
					            this.opener.reportError("Unable to update at this time. Your attachment was not saved.", false, data.error || data.msg);
					            return;
					        }

					        cbData = sgCurrentRepoUrl + "/wiki/image/" + data.recid + "/" + encodeURIComponent(data.filename);
					    },

					    error: function (data, status, e)
					    {
					        this.pop.vvdialog("vvclose");
					        this.opener.reportError("Unable to update at this time. Your attachment was not saved.", false);
					    }
					});
			});

        adiv.vvdialog({
            "title": "Upload an Image",
            "resizable": false,
            "width": 400,
            "height": 100,

            close: function (event, ui)
            {
                if (cb)
                    cb(null);
                $("#backgroundPopup").hide();
            }
        });


        return (true);
    },

    _hookUpTextEvents: function ()
    {
        var self = this;

        this.textedit.droppable(
			{
			    drop: function (e, ui)
			    {
			        var dragged = ui.draggable;

			        if (dragged[0].nodeName.toLowerCase() == 'li')
			        {
			            var rel = dragged.data('rel');

			            var oldVal = self.textedit.val();

			            if (rel.id)
			            {
			                self.textedit.val(oldVal + ' ^' + rel.id);
			                self.editor.refreshPreview();
			            }
			            else if (rel.commit)
			            {
			                var prefix = '';

			                if (rel.repo && (rel.repo != startRepo))
			                    prefix = rel.repo + ":";
			                self.textedit.val(oldVal + ' @' + prefix + rel.commit);
			                self.editor.refreshPreview();
			            }

			            dragged.css('position', 'relative');
			            return (false);
			        }
			    },

			    hoverClass: 'relhover',
			    activeClass: 'relactive',

			    accept: function (tr)
			    {
			        var rel = tr.data('rel');
			        if (rel && (rel.other || rel.commit))
			            return (true);

			        return (false);
			    }
			}
		);
        this.textedit.keypress(function (e)
        {
            var ch = e.charCode || e.keyCode || e.which || 0;

            if (ch == 94)			// ^
            {
                self.promptForBug(true);

                return (false);
            }
            else if (ch == 64)		// @
            {
                self.promptForCs(true);

                return (false);
            }
        }
		);

    },
    promptForBug: function (charStart)
    {
        if (document.selection)
        {
            this.textedit[0].focus();
            this.trange = document.selection.createRange();
        }
        this.witCharStart = !!charStart;

        this.wipop.dialog("open");
        this.searchinp.focus();
    },

    promptForCs: function (charStart)
    {
        if (document.selection)
        {
            this.textedit[0].focus();
            this.trange = document.selection.createRange();
        }

        this.csCharStart = !!charStart;

        this.cspop.dialog("open");
        this.cssearchinp.focus();
    },

    promptForFilter: function ()
    {
        if (document.selection)
        {
            this.textedit[0].focus();
            this.trange = document.selection.createRange();
        }

        this.filterpop.dialog("open");
        this.filtersearchinp.focus();
    },
	
    promptForWiki: function ()
    {
        if (document.selection)
        {
            this.textedit[0].focus();
            this.trange = document.selection.createRange();
        }

        this.wikipop.dialog("open");
        this.wikisearchinp.focus();
    }
});
