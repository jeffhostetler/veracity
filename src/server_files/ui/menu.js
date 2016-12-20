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

var _menuWorkItemHover = null;
var noRepo = false;
var gotRepos = false;
var $aboutdialog = null;
var topMenuBound = false;
var visitedRepoCookieName = "sgVisitedRepos";
var VERACITY_DOWNLOAD_LINK = "http://veracity-scm.com/downloads.html";

function menuSetup()
{
    var faMenu = $('#menufilteractivity');
    $("#menushowactivity").click(_menuShowHideActivity);
    $("#menufilteractivity").click(_menuToggleFilterActivity);
    if (window._showActivity)
    {
        if (_showActivity())
        {
            $('#menushowactivity').addClass("checked");
        }
        else
        {
            $('#menushowactivity').removeClass("checked");
            faMenu.hide();
        }

        if (_filterActivity())
            faMenu.addClass('checked');
        else
            faMenu.removeClass('checked');
    }
    $(".menudropdownsub").unbind("click");
    $(".menudropdownsub").click(function (e)
    {
        _showMenu($(this), e);
    });

    if (sgLocalRepo && sgLocalRepo == startRepo)
    {
        $("#menu_local_status").show();
        $("#menu_local_fs").show();
    }
    if (!topMenuBound)
    {
        $(document).mouseup($(this), function (e)
        {
            if ($("li.menudropdown").hasClass("open"))
            {
                e.stopPropagation();
                if (!$(e.target).hasClass("submenuli") && !$(e.target).hasClass("menulink") && !$(e.target).hasClass("imgmenudropdown")
                        && !$(e.target).hasClass("menudropdown") && !$(e.target).hasClass("menudropdownsub"))
                {
                    $("ul.submenu").hide();
                    $("li.menudropdown").removeClass("open");

                    return false;
                }
                else if ($(e.target).hasClass("menudropdownsub") || $(e.target).hasClass("imgmenudropdown"))
                {
                    var menus = $("li.menudropdown.open");

                    $.each(menus, function (i, v)
                    {
                        if ($(v).attr("id") != $(e.target).parents("li").attr("id"))
                        {
                            $(v).find("ul.submenu").hide();
                            $(v).removeClass("open");
                        }
                    });

                }
            }

        });
        topMenuBound = true;

    }
}

function _showMenu(control, e)
{
    var sub = control.parent().find("ul");

    sub.css({ "min-width": Math.max(control.parent().width(), sub.width()) + 5 });

    if (control.parent().hasClass("open"))
    {
        sub = control.parent().find("ul.submenu");
        sub.hide();
        control.parent().removeClass("open");
    }
    else
    {
        control.parent().addClass("open");
        sub.slideDown('fast');

    }

}


function _setupAbout()
{
    $aboutdialog = $('<div>').attr("id", "divAboutDialog").addClass("primary").css({ "dislpay": "block", "background-color": "#f6f8ef", "text-align": "center" })
	.dialog({
	    buttons:
        [{
            id: "buttonAboutOK",
            text: "OK",
            click: function ()
            {
                $(this).dialog("close");
            }
        }],

	    width: 500,
	    autoOpen: false,
	    title: 'About Veracity',
	    resizable: false
	});

}
function _menuShowAbout(e)
{
    e.preventDefault();

    $aboutdialog.dialog("open");
    $('#buttonAboutOK').focus();
    vCore.ajax(
		{
		    type: "GET",
		    url: sgLinkRoot + "/version.txt",

		    success: function (data, status, xhr)
		    {
		        if (!$('#about_versiondiv').length)
		        {
		            var versionDiv = $('<div id="about_versiondiv">Veracity ' + data + "</div>");
		            var p = $("<p>").attr("id", "pDownloadLink");

		            var downloadLink = $('<a>').css("color", "#667a30").text("Download Veracity").attr("href", VERACITY_DOWNLOAD_LINK);
		            p.html(downloadLink);
		            $aboutdialog.append(versionDiv);
		            $aboutdialog.append(p);
		            $aboutdialog.append($('<p>Copyright &#169; ' + (new Date()).getFullYear() + ' SourceGear, LLC. All rights reserved. </p>'));
		        }
		    }

		}
	);

}

function _menuShowHideActivity()
{
    if (!window._showActivity)
        return;

    if (_showActivity())
    {
        hideActivity();
        _setShowActivity(false);
    }
    else
    {
        displayActivity();
        _setShowActivity(true);
        listActivity();
    }
}

function _menuToggleFilterActivity()
{
    if (window._filterActivity && _filterActivity())
    {
        _filterActivity(false);
    }
    else
    {
        _filterActivity(true);
    }

    if (window._showActivity && _showActivity())
    {
        _vvact.spin();
        listActivity();
    }
}

function _menuClearSearch(e)
{
    e.preventDefault();
    e.stopPropagation();
    $('#textmenusearch').categorycomplete("close");
    $('#textmenusearch').categorycomplete("clearTerm");
    $('#textmenusearch').val("");

    $('#textmenusearch').removeClass("ui-autocomplete-loading");
    //todo cancel request??
}


$.widget("custom.categorycomplete", $.ui.autocomplete,
{
    fromEnter: false,
    ajaxRequest: null,
    focusedItem: null,

    setFocusedItem: function (value)
    {
        this.focusedItem = value;
    },
    getFocusedItem: function ()
    {
        return this.focusedItem;
    },
    setEnter: function (value)
    {
        this.fromEnter = value;
    },
    clearTerm: function ()
    {
        this.term = "";
        if (this.ajaxRequest)
        {
            this.ajaxRequest.abort();
        }
    },
    _renderMenu: function (ul, items)
    {
        self = this;
        $("#divSearchArea .ui-menu").unbind("click");
        $("#divSearchArea .ui-menu").click(function (event)
        {
            if (!$(event.target).closest(".ui-menu-item a").length)
            {
                return;
            }
            if (event.which == 2)
            {
                $(document).one('mousedown', function (event)
                {
                    var menuElement = $("#divSearchArea .ui-autocomplete");
                    if ((event.target !== self.element[0]) && (event.target !== menuElement) && !menuElement.find(event.target).length)
                    {
                        self.close();
                    }
                });

            }
        });


        var self = this;
        var currentCategory = "";
        $.each(items, function (index, item)
        {
            if (item.category != "more" && item.category != "none" && item.category != currentCategory)
            {
                ul.append("<li class='ui-autocomplete-category'>" + item.category + "</li>");
                currentCategory = item.category;
            }
            self._renderItem(ul, item);
        });
    },
    _renderItem: function (ul, item)
    {
        var li = $("<li></li>")
			.data("item.autocomplete", item)
            .appendTo(ul);

        if (item.category == "none")
        {
            li.text(item.label);
        }
        else
        {
            var url = getSearchItemURL(item);
            li.append($("<a></a>").attr("href", url).text(item.label));
        }
        return li;
    }

});

function getSearchItemURL(item)
{
    var url = "";
    if (item.category == "changesets")
    {
        url = sgCurrentRepoUrl + "/changeset.html?recid=" + encodeURIComponent(item.id);
    }
    else if (item.category == "more")
    {
        url = sgCurrentRepoUrl + "/search.html?text=" + encodeURIComponent(item.id) + "&areas=wit,vc,builds,wiki";
    }
    else if (item.category == "builds")
    {
        url = sgCurrentRepoUrl + "/build.html?recid=" + encodeURIComponent(item.id);
    }
    else if (item.category == "wiki")
    {
        url = sgCurrentRepoUrl + "/wiki.html?page=" + encodeURIComponent(item.id);
    }
    else
    {
        url = sgCurrentRepoUrl + "/workitem.html?recid=" + encodeURIComponent(item.id);
    }
    return url;

}

function _setUpSearch()
{
    $('#textmenusearch').keyup(function (e1)
    {
        if (e1.which == 13)
        {
            e1.preventDefault();
            e1.stopPropagation();
            var item = $(this).categorycomplete("getFocusedItem");
            if (!item)
            {
                $(this).categorycomplete("setEnter", true);
                $(this).categorycomplete("search");
            }
            else
                window.location.href = getSearchItemURL(item);
        }

    });
    $("#menuSearchAdvanced").click(function (e)
    {
        if ($('#textmenusearch').val())
        {
            var url = sgCurrentRepoUrl + "/search.html?areas=wit,vc,builds,wiki";

            url += "&text=" + encodeURIComponent($('#textmenusearch').val());
            $("#menuSearchAdvanced").attr("href", url);

        }
    });

    $('#textmenusearch').categorycomplete({

        source: function (request, response)
        {
            var self = this;

            vCore.ajax({
                url: sgCurrentRepoUrl + "/searchresults.json?text=" + encodeURIComponent(request.term),

                beforeSend: function (jqXHR)
                {
                    var pending = self.pending;
                    if (self.ajaxRequest)
                    {
                        pending--;
                        self.pending = Math.max(pending, 0);
                        self.ajaxRequest.abort();
                    }
                    self.ajaxRequest = jqXHR;
                },
                success: function (data)
                {
                    self.pending = 0;
                    $('#textmenusearch').removeClass("ui-autocomplete-loading");
                    if (data.error)
                    {
                        reportError("invalid input - " + data.error);
                        return;
                    }
                    var items = [];
                    var inList = {};
                    $.each(data.items, function (i, v)
                    {
                        if (!inList[v.id])
                        {
                            items.push(v);
                            inList[v.id] = v;
                        }
                    });


                    if (self.fromEnter)
                    {
                        if (items.length == 1)
                        {
                            if (items[0].category == "changesets")
                            {
                                url = sgCurrentRepoUrl + "/changeset.html?recid=" + items[0].id;
                            }
                            else if (items[0].category == "builds")
                            {
                                url = sgCurrentRepoUrl + "/build.html?recid=" + items[0].id;
                            }
                            else if (items[0].category == "work items")
                            {
                                url = sgCurrentRepoUrl + "/workitem.html?recid=" + items[0].id;
                            }
                            else if (items[0].category == "wiki")
                            {
                                url = sgCurrentRepoUrl + "/wiki.html?page=" + encodeURIComponent(items[0].title);
                            }
                            document.location.href = url;
                            self.fromEnter = false;
                            return;
                        }

                    }
                    var lastResult = null;
                    if (!items.length)
                    {
                        lastResult =
                        {
                            name: "no results",
                            category: "none"
                        };
                    }
                    else if (data.more)
                    {
                        lastResult =
                        {
                            name: "more results...",
                            category: "more"
                        };
                    }
                    else
                    {
                        lastResult =
                        {
                            name: "detailed results...",
                            category: "more"
                        };
                    }
                    items.push(lastResult);

                    response($.map(items, function (item)
                    {
                        if (item.category == "builds")
                        {
                            var d = new Date(parseInt(item.extra));
                            var dateStr = shortDateTime(d); // d.strftime("%Y/%m/%d %H:%M%P");
                            return { label: item.name + " - " + dateStr + " - " + item.description, value: item.name, id: item.id, category: item.category };

                        }
                        else if (item.category == "wiki")
                        {
                            return { label: item.name, category: item.category, id: item.id, value: item.id };
                        }
                        else if (item.category == "more" || item.category == "none")
                        {
                            return { label: item.name, category: item.category, id: request.term };
                        }
                        else
                        {
                            return { label: item.name + " - " + item.extra + " - " + item.description, value: item.name, id: item.id, category: item.category };

                        }
                    }));
                    self.fromEnter = false;
                }
            });
        },
        minLength: 2,
        delay: 1000,
        position:
			{
			    my: "right top",
			    at: "right bottom"
			},
        appendTo: "#divSearchArea",
        focus: function (event, ui)
        {
            $(this).categorycomplete("setFocusedItem", ui.item);
        }

    }
	);
}

function _visitedFilters()
{
    var cname = vCore.getDefaultCookieName();
    return vCore.getCookieValueForKey(cname, "visitedFilters", []);
}

function _findFilter(filters, recid)
{
    for (var i = 0; i < filters.length; i++)
    {
        if (filters[i].recid == recid)
        {
            return { index: i, filter: filters[i] };
        }
    }
    return null;
}

function _saveVisitedFilter(name, recid)
{
    var cname = vCore.getDefaultCookieName();
    var visited = _visitedFilters();
    var items = [];
    var ap = null;
    if (visited)
    {
        ap = _findFilter(visited, recid);
        if (!ap && visited.length > 4)
        {
            visited.pop();
        }

        if (ap)
        {
            visited.splice(ap.index, 1);
        }
    }
    visited.unshift({ "name": name, "recid": recid });
    vCore.setCookieValueForKey(cname, "visitedFilters", visited, 30);
    _setFilterMenu();
}

function _createFilterMenuList(data)
{
    var ul = $("#filtersmenu");
    $('#filtersmenu li').remove();

    var liList = [];
    $.each(data, function (i, v)
    {
        if (i < 5)
        {
            var add = false;
            var addCount = 0;
            var li = $("<li>").addClass("submenuli");
            var a = $("<a>").text(v.name).attr({ href: sgCurrentRepoUrl + "/workitems.html?filter=" + v.recid });
            a.addClass("menulink");
            li.append(a);

            ul.append(li);
        }
    });

    //we need to do this after the filter menu has been created
    //so all of the events get bound
    _bindLiClickEvents($(document));

}

function _deleteFromFiltersMenu(recid, afterDeleteFunction)
{
    var cname = vCore.getDefaultCookieName();
    var visited = _visitedFilters();

    var ap = null;
    if (visited)
    {
        ap = _findFilter(visited, recid);

        if (ap)
        {
            visited.splice(ap.index, 1);
        }

        //fetch more saved filter values so we always have
        //10 items in the menu
        if (visited.length < 5)
        {
            var count = visited.length;
            vCore.fetchFilters(
            {
                max: 5,
                success: function (data)
                {
                    for (var i in data)
                    {
                        if (!_findFilter(visited, data[i].recid))
                        {
                            _saveVisitedFilter(data[i].name, data[i].recid);

                            if (count++ == 5)
                                break;
                        }
                    }
                    if (afterDeleteFunction)
                        afterDeleteFunction();
                }
            });
        }
        else
        {
            if (afterDeleteFunction)
                afterDeleteFunction();
        }
    }
    vCore.setCookieValueForKey(cname, "visitedFilters", visited, 30);
    _setFilterMenu();

}

function _addFiltersToMenu(data)
{
    $.each(data, function (i, v)
    {
        if (i < 5)
        {
            _saveVisitedFilter(v.name, v.recid);
        }
    });

    _createFilterMenuList(data);
}

function _setFilterMenu(forceRefresh)
{
    var visited = _visitedFilters();
    var items = [];

    if (sgUserName && sgUserName.length > 0)
    {
        $("#menumanagefilters").show();
        if (!forceRefresh && (visited && visited.length > 0))
        {
            _createFilterMenuList(visited);
        }
        else
        {
            if (forceRefresh)
            {
                //clear saved filter cookie
                var cname = vCore.getDefaultCookieName();
                vCore.setCookieValueForKey(cname, 'visitedFilters', [], 30);
            }
            vCore.fetchFilters({ max: 5, success: _addFiltersToMenu });
        }
    }
}

var sgFilterResultsCookie = "sgfilterresults";

function _saveLastQuerySession(value)
{
    //return _findCookie(sgQueryResultsCookie);
    var url = sgCurrentRepoUrl + "/filteredworkitems/sglastqueryurl.json";

    vCore.ajax(
		{
		    type: "POST",
		    url: url,
		    data: JSON.stringify({ url: value })
		}
	);
}

function redirLastQuerySession()
{
    _getLastQuerySession(true);

}
function _getLastQuerySession(redir)
{
    //return _findCookie(sgQueryResultsCookie);
    var url = sgCurrentRepoUrl + "/filteredworkitems/sglastqueryurl.json";

    vCore.ajax(
		{
		    type: "GET",
		    url: url,
		    success: function (data, status, xhr)
		    {
		        if (redir)
		            window.location.href = data;
		        return data;
		    }
		}
	);
}


function _visitedItems()
{
    var cname = vCore.getDefaultCookieName();
    return vCore.getCookieValueForKey(cname, "visitedItems", []);
}


function _displayLastVisitedItems()
{
    var cookie = _visitedItems();

    if (cookie)
    {
        $.each(cookie, function (i, v)
        {
            //backwards compatability (we used to store recids too)
            if (v.length < 65)
            {
                var li = $("<li>").attr("id", v).addClass("submenuli");
                var a = $("<a>").text(v).attr({ href: sgCurrentRepoUrl + "/workitem/bugid/" + v });
                a.addClass("menulink");

                li.append(a);
                $("#visitedmenu").append(li);
                _menuWorkItemHover.addHover(li, v);
            }

        });
    }

}

function _saveVisitedItem(id)
{
    var cname = vCore.getDefaultCookieName();
    var visited = _visitedItems();
    if (visited && visited.length > 5)
    {
        visited.pop();
    }

    if ($.inArray(id, visited) < 0)
    {
        visited.unshift(id);
        vCore.setCookieValueForKey(cname, "visitedItems", visited, 30);
    }
    $("#visitedmenu").find("li").remove();
    _displayLastVisitedItems();
}

var branchesDisplayed = false;
function _displayLastVisitedBranches(visited, force)
{
    if (branchesDisplayed && !force)
    {
        return;
    }
    visited = visited || vCore.getCookieValueForKey(vCore.getDefaultCookieName(), "visitedBranches") || [];

    var len0 = visited.length; // Save original length to know if we deleted something, in which case we'll update the cookie.
    var masterShown = false;
    var i = 0;
    while (i < visited.length)
    {
        var branchName = visited[i];
        if (!sgOpenBranches[branchName])
        {
            visited.splice(i, 1);
        }
        else
        {
            var li = $("<li>").addClass("submenuli");
            var a = $("<a>").text(branchName).attr({ href: branchUrl(branchName) });
            a.addClass("menulink");
            li.append(a);
            $("#visitedbranchmenu").append(li);
            if (branchName == "master") { masterShown = true; }
            ++i;
        }
    }
    if (visited.length < 5 && !masterShown && sgOpenBranches["master"])
    {
        var li = $("<li>").addClass("submenuli");
        var a = $("<a>").text("master").attr({ href: branchUrl("master") });
        a.addClass("menulink");
        li.append(a);
        $("#visitedbranchmenu").append(li);
    }
    if (visited.length < len0)
    {
        vCore.setCookieValueForKey(vCore.getDefaultCookieName(), "visitedBranches", visited, 30);
    }
    branchesDisplayed = true;
}
function _saveVisitedBranch(name)
{
    if (sgOpenBranches[name])
    {
        var visited = vCore.getCookieValueForKey(vCore.getDefaultCookieName(), "visitedBranches") || [];
        var i = $.inArray(name, visited);
        if (i != 0)
        {
            if (i > -1)
            {
                visited.splice(i, 1);
            }
            visited.unshift(name);
            if (visited.length > 5)
            {
                visited.pop();
            }
            vCore.setCookieValueForKey(vCore.getDefaultCookieName(), "visitedBranches", visited, 30);
            $("#visitedbranchmenu").find("li").remove();
            _displayLastVisitedBranches(visited, true);
        }
    }
}



function _findCurrentSprint()
{
    var url = sgCurrentRepoUrl + "/config/current_sprint";

    vCore.ajax(
		{
		    type: "GET",
		    url: url,
		    success: function (data, status, xhr)
		    {
		        sgCurrentSprint = data.value;
		    }

		}
	);
}


function deleteFromRepoMenu(reponame, afterDelete)
{
    var visited = vCore.getJSONCookie(visitedRepoCookieName);
    if (!visited)
        visited = [];

    if (visited && visited.length)
    {
        var pos = $.inArray(reponame, visited);
        if (pos >= 0)
        {
            visited.splice(pos, 1);
        }
    }
    vCore.setCookie(visitedRepoCookieName, JSON.stringify(visited), 30);
    _displayLastVisitedRepos(afterDelete);
}
function _saveVisitedRepo(reponame)
{
    var visited = vCore.getJSONCookie(visitedRepoCookieName) || [];

    var p = $.inArray(reponame, visited);

    if (p >= 0)
        visited.splice(p, 1);

    if (visited.length > 5)
        visited.pop();

    visited.unshift(reponame);

    vCore.setCookie(visitedRepoCookieName, JSON.stringify(visited), 30);
}

function _displayLastVisitedRepos(afterfunction)
{
    if (sgLocalRepo && sgLocalRepo == startRepo)
    {
        _setMenuRepos([startRepo], startRepo, true);
        $("#spanRepoName").addClass("no-arrow");
        menuSetup();
        if (afterfunction)
            afterfunction();
    }
    else
    {
        if ($("#spanRepoName").length)
        {
            vCore.fetchRepos(
				{
				    success: function (data)
				    {
				        var visitedRepoCookieName = "sgVisitedRepos";
				        if (!data.length)
				        {
				            noRepo = true;
				            vCore.setCookie(visitedRepoCookieName, "", -1);
				        }
				        var visited = vCore.getJSONCookie(visitedRepoCookieName) || [];
				        if (!visited[0])
				        {
				            _setMenuRepos(data);
				            var initRepos = data;
				            if (data.length > 5)
				                initRepos = data.slice(0, 5);
				            vCore.setCookie(visitedRepoCookieName, JSON.stringify(initRepos), 30);
				        }
				        else
				        {
				            var tmpVisited = visited;
				            $.each(visited, function (i, v)
				            {
				                //see if the visited repo is still in the list
				                //of fetched repos - remove it if not
				                if ($.inArray(v, data) < 0)
				                {
				                    index = $.inArray(v, tmpVisited);
				                    tmpVisited.splice(index, 1);
				                }

				            });
				            $.each(data, function (i, v)
				            {

				                //add any new repos
				                if (v && ($.inArray(v, tmpVisited) < 0) && (tmpVisited.length < 6))
				                    tmpVisited.push(v);

				            });

				            _setMenuRepos(tmpVisited);
				        }
				        if (afterfunction)
				        {
				            afterfunction();
				        }
				    },
				    complete: menuSetup

				});

        }
        else
        {
            menuSetup();
        }
    }
}

function truncateRepoName(name)
{
    if (name.length > 50)
    {
        return name.slice(0, 50) + "...";
    }
    return name;
}

function _setMenuRepos(repos, hideManage)
{

    $("#ulmenurepos").find("li").remove();
    $("#linkallrepos").remove();
    var currRepoText = $("#spanRepoName").text();

    $("#spanRepoName").text(truncateRepoName(currRepoText));

    if (repos.length)
    {
        $('#menureposelector').addClass("menudropdown repos");
        $("#imgrepodropdown").show();
        var n = 0;

        $.each(repos, function (i, v)
        {
            if (v != startRepo && n < 6)
            {
                var url = sgLinkRoot + "/repos/" + encodeURIComponent(v);

                if (window.vvredirectable)
                    url += "/" + window.vvredirectable;

                var li = $("<li class='submenuli'></li>");

                var a = $("<a>").text(truncateRepoName(v)).attr({ "href": url, "title": v }).addClass("menulink").appendTo(li);

                $("#ulmenurepos").append(li);
                n++;
            }
        });

    }
    else
    {
        $("#imgrepodropdown").hide();
        $(".needrepo").hide();
        $("#spanRepoName").hide();

    }
    //hide the manage repos link if the server was started from within a working directory
    if (!hideManage)
    {
        var a = $("<a>").text("Repositories...").attr("id", "linkallrepos").addClass("allRepos");

        if (repos.length && sgCurrentRepoUrl)
        {
            a.attr({ "href": sgCurrentRepoUrl + "/repos.html", "title": "view all repositories" }).addClass("menulink");
        }
        else
        {
            a.attr({ "href": sgLinkRoot + "/repos.html", "title": "view all repositories" }).addClass("menulink");
        }

        if ($("#spanRepoName").length)
        {
            if (!repos.length)
            {
                $("#menureposelector").append(a);
                $("#spanRepoName").hide();
            }
            else
            {
                var li = $("<li>").addClass("allRepos");
                $("#ulmenurepos").append($('<li class="separator allrepos"></li>'));
                li.append(a).addClass("submenuli");
                $("#ulmenurepos").append(li);
            }
        }
    }
    //$('<span></span>').text(repos[0]).prependTo($("#topMenuLiChooseRepo"));
    //we need to do this after the repos menu has been created
    //so all of the events get bound
    _bindLiClickEvents($("#ulmenurepos"));


}

function _bindLiClickEvents(parentMenuObj)
{
    parentMenuObj.find(".submenuli:not(.local)").unbind("click");

    parentMenuObj.find(".submenuli:not(.local)").click(function (e)
    {
        if ($(e.target).hasClass("submenuli"))
        {
            var a = $(this).find("a.menulink");
            if (a && a.length && $(a).attr("href") != "#")
            {
                if (a.hasClass("new"))
                {
                    window.open($(a).attr("href"));
                }
                else
                {
                    window.location.href = $(a).attr("href");
                }
            }
        }
    });
}

$(document).ready(function ()
{
    _setupAbout();
    $("#liMenuAbout").click(_menuShowAbout);

    _setUpSearch();
    _menuWorkItemHover = new hoverControl(fetchWorkItemHover);
    _displayLastVisitedRepos(function ()
    {
        if (!noRepo)
        {
            _saveVisitedRepo(startRepo);
        }
        if (sgUserName && sgUserName.length > 0 && sgUserName != "undefined")
        {
            if (!noRepo)
            {
                _displayLastVisitedBranches();
                _displayLastVisitedItems();
                _setFilterMenu();

                $(".needrepo").show();
            }
        }
        else
        {
            $("#useremail").text("(not logged in)");
        }
        $("#verytop>ul").show();
        $("#topmenu").show();

    });


    vCore.setMenuHeight();

    $("#menuclosesearch").click(_menuClearSearch);
});

//
//context menus
//
var highlightedRow = null;
var mouseOverRow = null;
function enableRowHovers(preventCtxMenu, selector)
{
    var htrs = selector || $("table.datatable tr:not(.filtersectionheader, .nohover, .buildadder, .tr-newwi, .tablegroupsep)");
    if (highlightedRow)
    {
        highlightedRow.children('td').css("background-color", "#FFF");
        highlightedRow.find(".linkSpan").first().hide();
    }
    highlightedRow = null;
    //htrs.unbind("mouseover").unbind("mouseout");
    htrs.unbind("hover");
    htrs.hover(function ()
    {
        highlightedRow = $(this);

        $(this).children('td').addClass('hovered').removeClass('unhovered');

        if (preventCtxMenu !== true && !isTouchDevice())
        {
            $(this).find(".linkSpan").first().show();
        }

    },
    function ()
    {
        $(this).children('td').addClass('unhovered').removeClass('hovered');

        if (preventCtxMenu !== true && !isTouchDevice())
        {
            $(this).find(".linkSpan").first().hide();
        }
    });


    if (preventCtxMenu === true)
        htrs.find("span.linkSpan").first().hide();
}

function disableRowHovers()
{
    var trs = $("table.datatable tr");
    trs.unbind("hover");

}

//relativeToElementId - dom emelement to use to position context menu also used as default "recid" for menu data if optData
//                      isn't specified
//menuObj - the actual context menu, if not specified $("#editMenu") will be used
//optData - extra data needed in the form {"id":"value", "id2: "value2", etc} if nothing passed in will use relativeToElementId data
function showCtxMenu(relativeToElementId, menuObj, optData)
{
    var menu = menuObj || $("#editMenu");
    var el = $("#" + relativeToElementId);
    var pos = el.offset();
    var width = el.width();
    if (optData)
    {
        menu.data(optData);
    }
    else
    {
        menu.data("recid", relativeToElementId);
    }

    menu.hide();


    menu.css(
		{
		    "position": "absolute",
		    "left": (pos.left + width) + "px",
		    "top": pos.top + "px"
		}
	);

    disableRowHovers();

    $(document).bind("mouseup.contextmenu", function (e)
    {
        // e.preventDefault();
        e.stopPropagation();
        if (!$(e.target).hasClass("ctxMenuItem") && !$(e.target).hasClass("ctxMenuItemImg"))
        {
            $(document).unbind("mouseup.contextmenu");
            enableRowHovers();

            menu.hide();
            if ($("#delConfMenu"))
            {
                $("#delConfMenu").hide();
            }
            if ($(".filterContextSubmenu"))
            {
                $(".filterContextSubmenu").hide();
            }
            return false;
        }

        return true;
    });

    menu.fadeIn();
    var height = menu.find("ul").height();
    if (pos.top + menu.find("ul").height() + 10 >= $(window).height() + $(window).scrollTop())
    {
        var top = (pos.top + 16) - height;
        menu.css(
		{
		    "top": top + "px"
		});
    }
}

function showDeleteSubMenu(recid)
{
    var menu = $("#delConfMenu");
    var el = $("#confirm_delete");
    var pos = el.offset();
    var width = el.outerWidth();

    menu.data("recid", recid);

    menu.hide();

    menu.css(
		{
		    "position": "absolute",
		    "left": (pos.left + width) + "px",
		    "top": (pos.top - 1) + "px"
		}
	);

    menu.fadeIn();
}

function showContextSubMenu(recid, parent, menu)
{
    var pos = parent.offset();
    var width = parent.outerWidth();

    menu.data("recid", recid);

    menu.css(
		{
		    "position": "absolute",
		    "left": (pos.left + width) + "px",
		    "top": (pos.top - 1) + "px"
		}
	);

    menu.fadeIn();
    var height = menu.find("ul").height();
    if (pos.top + menu.find("ul").height() + 10 >= $(window).height() + $(window).scrollTop())
    {
        var top = (pos.top + 19) - height;
        menu.css(
		{
		    "top": top + "px"
		});

    }
}

/* set highlighted section in top menu */
function setCurrentSection(section)
{
    var secid = '#topmenu' + section;

    $(document).ready(
		function ()
		{
		    $('#top li').removeClass('currentsection');
		    $(secid).addClass('currentsection');
		});
}