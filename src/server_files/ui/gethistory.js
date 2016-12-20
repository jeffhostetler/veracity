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

var earliestRevShown = 0;
var historyItems = [];
var stampsLoaded = false;
var usersLoaded = false;
var loadingHistory = false;
var hg = new historyGraph();
var allRetrieved = false;
var state = {};
var maxRows = 50;
var branches = null;
var branchesLoaded = false;
var lastCSIDShown = "";
var scrollPosition = 0;
var more = false;
var noUpdate = false;
var vvInactiveUsers = null;

function resetHistory()
{
    $('#tablehistory td').remove();
    allRetrieved = false;
    allLoaded = false;
    historyItems = [];
    hg.clearGraph();
    lastCSIDShown = "";
    state = {};
    more = false;
}

function getQueryStringPair(qs, key, value)
{
    var str = "";
    str = (qs.length == 0 ? "?" : "&") + key + "=" + value;
    return str;
}

function showInactiveUsers(show)
{
    if (show)
    {
        if (vvInactiveUsers)
        {
            var u = $('#user');

            $.each(vvInactiveUsers,
				function (i, v)
				{
				    u.append(v);
				}
			);
        }
    }
    else
        if (vvInactiveUsers)
            vvInactiveUsers.detach();
}

function setPageFromState()
{
    //if we have something in the state we know
    //we did a query so show the filter
    if (!$('#filter').is(":visible"))
    {
        $('#toggle_filter').click();
    }

    var st = $.bbq.getState();

    $('#user').val(st["user"]);

    if (st["path"])
    {
        $('#txtPath').val(st["path"]);
        $("#cbxHideMerges").removeAttr("disabled");
        $("#labelHideMerges").removeClass("disabled");
        if (st["hideMerges"] == "true")
        {
            $("#cbxHideMerges").attr("checked", "checked");
        }
    }
    $('#datefrom').val("");
    if (st["from"])
    {
        var from = formatDateFromQuery(st["from"]);
        $('#datefrom').val(from);
    }
    if (st["to"])
    {
        var to = formatDateFromQuery(st["to"]);
        $('#dateto').val(to);
    }
    $('#stamp').val(st["stamp"] || "");

    if (st["allbranches"] == "true")
    {
        $("#cbxShowAllBranches").attr("checked", "checked");
        populateBranchList(true);
    }
    if (st["inactive"] == "true")
    {
        $("#cbxShowAllUsers").attr("checked", "checked");

        showInactiveUsers(true);
    }
    else
    {
        $("#cbxShowAllUsers").removeAttr("checked");
        showInactiveUsers(false);
    }

    $('#selectBranch').val(st["branch"]);
}

function setFlags()
{
    var st = $.bbq.getState();
    var mask = ~(FLAG_STAMP_FILTER | FLAG_NAME_FILTER | FLAG_FROM_FILTER | FLAG_TO_FILTER | FLAG_BRANCH_FILTER | FLAG_PATH_FILTER);
    filterflags &= mask;
    if (st["from"] || getQueryStringVal("from"))
    {
        filterflags |= FLAG_FROM_FILTER;
    }

    if (st["to"] || getQueryStringVal("to"))
    {
        filterflags |= FLAG_TO_FILTER;
    }

    if (st["user"] || getQueryStringVal("user"))
    {
        filterflags |= FLAG_NAME_FILTER;
    }

    if (st["stamp"] || getQueryStringVal("stamp"))
    {
        filterflags |= FLAG_STAMP_FILTER;
    }

    if (st["branch"] || getQueryStringVal("branch"))
    {
        filterflags |= FLAG_BRANCH_FILTER;
    }

    if (st["path"] || getQueryStringVal("path"))
    {
        filterflags |= FLAG_PATH_FILTER;
    }
}


function hashStateChanged(e)
{
    if (!loadingHistory && !noUpdate)
    {
        resetHistory();
        getHistory(maxRows);
    }
    noUpdate = false;
}

function formatDateFromQuery(strDate)
{
    try
    {
        var tmp = new Date(parseInt(strDate));
        return tmp.strftime("%Y/%m/%d");
    }
    catch (ex)
    {
        return "";
    }
}
function formatDateForQuery(strDate, setEndOfDay)
{
    try
    {
        var tmp = new Date(strDate);
        if (setEndOfDay)
        {
            tmp.setDate(tmp.getDate() + 1);
            tmp.setMilliseconds(-1);
        }
        if (tmp && tmp != "Invalid Date")
        {
            /*return tmp.strftime("%Y-%m-%d");*/
            return tmp.getTime();
        }
        else
        {
            reportError("Invalid date format. Dates should have the format: YYYY/MM/DD.");
            return false;
        }
    }
    catch (e)
    {
        reportError("Invalid date format. Dates should have the format: YYYY/MM/DD.");
        return false;
    }

}

function filterHistory()
{
    state = {};
    var from = $('#datefrom').val();

    if (from)
    {
        from = formatDateForQuery(from);
        if (!from)
        {
            return;
        }
        state["from"] = from;
    }
    else
    {
        delete state.from;
        $.bbq.removeState("from");
    }
    var to = $('#dateto').val();

    if (to)
    {
        to = formatDateForQuery(to, true);
        if (!to)
        {
            return;
        }
        state["to"] = to;
    }
    else
    {
        delete state.to;
        $.bbq.removeState("to");
    }

    var user = $('#user').val();
    if (user && user != 0)
    {
        state["user"] = user;
    }
    else
    {
        delete state.user;
        $.bbq.removeState("user");
    }
    var stamp = $('#stamp').val();
    if (stamp)
    {
        state["stamp"] = stamp;
    }
    else
    {
        delete state.stamp;
        $.bbq.removeState("stamp");
    }

    var branch = $('#selectBranch').val();
    if (branch && branch != "0")
    {
        state["branch"] = branch;
    }
    else
    {
        delete state.branch;
        $.bbq.removeState("branch");
    }

    var path = $('#txtPath').val();
    if (path)
    {
        state["path"] = path;
    }
    else
    {
        delete state.path;
        $.bbq.removeState("path");
    }

    if ($("#cbxHideMerges").is(':enabled'))
    {
        var merges = $("#cbxHideMerges").is(':checked');
        if (merges)
        {
            state["hidemerges"] = true;
        }
        else
        {
            delete state.hidemerges;
            $.bbq.removeState("hidemerges");
        }
    }
    else
    {
        $.bbq.removeState("hidemerges");
    }

    $.bbq.pushState(state);

    $(window).trigger('hashchange');

}


function clearFields()
{
    $('#datefrom').val("");
    $('#dateto').val("");
    $('#user').val(0);
    $('#stamp').val("");
    $('#txtPath').val("");
    $('#selectBranch').val(0);
    if ($("#cbxHideMerges").is(':checked'))
    {
        $("#cbxHideMerges").click();
    }

    $("#cbxHideMerges").attr("disabled", true);
    $("#labelHideMerges").addClass("disabled");

}
function clearFilter()
{
    resetHistory();
    clearFields();
    var mask = ~(FLAG_STAMP_FILTER | FLAG_NAME_FILTER | FLAG_FROM_FILTER | FLAG_TO_FILTER | FLAG_BRANCH_FILTER | FLAG_PATH_FILTER);
    filterflags &= mask;
    earliestRevShown = 0;
    $.bbq.pushState({}, 2);
    $(window).trigger('hashchange');
}


function getUsers(selectedUser)
{
    var url = findBaseURL("history.html") + "/users/records";
    var user = selectedUser || "";
    _inputLoading($("#user"), true);

    vCore.fetchUsers(
    {
        justNames: true,
        error: function (xhr, textStatus, errorThrown)
        {
            _stopInputLoading($("#user"));
        },
        success: function (active, inactive)
        {
            if (!active && !inactive)
            {
                return;
            }
            active.sort(function (a, b)
            {
                return (a.name.compare(b.name));
            });
            inactive.sort(function (a, b)
            {
                return (a.name.compare(b.name));
            });
            var data = active.concat(inactive);
            usersLoaded = true;
            var select = $('#user');
            select.append($('<option>').text("all").val(0));
            var params = $.deparam.fragment();
            var showInactive = params["inactive"] == "true";
            $.each(data, function (i, v)
            {

                var option = $('<option>').text(v.name).val(v.name).attr({ id: v.recid });
                select.append(option);
                if (v.inactive == true)
                {
                    var displayName = v.name;
                    if(displayName.slice(-11)!=" (inactive)"){
                        displayName += " (inactive)";
                    }
                    else{
                        option.val(v.name.slice(0, -11));
                    }
                    option.text(displayName);
                    option.addClass("inactive");
                }
            });

            var user = $.deparam.fragment()["user"];
            if (user)
            {
                select.val(user);
            }
            _stopInputLoading($("#user"));

            vvInactiveUsers = $('.inactive');
            if (!showInactive)
                showInactiveUsers(false);

            hg.draw();
        }

    });

}

function getAllStamps()
{
    _inputLoading($("#stamp"), true);
    var str = "";

    var url = sgCurrentRepoUrl + "/stamps.json";

    var stamps = [];
    vCore.ajax({
        url: url,
        dataType: 'json',

        error: function (xhr, textStatus, errorThrown)
        {
            _stopInputLoading($("#stamp"));
        },
        success: function (data, status, xhr)
        {
            _stopInputLoading($("#stamp"));
            if ((status != "success") || (!data))
            {
                return;
            }
            stampsLoaded = true;
            $.each(data, function (i, v)
            {
                stamps.push(v.stamp);
            });
            $("#stamp").autocomplete({
                source: stamps,
                minLength: 2
            });

            hg.draw();
        }

    });

}

function loadHistory(historyItems)
{
    if (historyItems.length == 0)
        return;

    $(historyItems).each(function (index, value)
    {
        var tr = $('<tr class="trReader"></tr>').attr("id", value.changeset_id);
        var tdt = $('<td>').addClass("tags");
        var tdbranch = $('<td>');
        var tdwhen = $('<td>');
        var tdwho = $('<td>');
        var tdcomment = $('<td>').attr("id", index).addClass("commentcell");
        var tdlink = $('<td>').css('padding-left', '5px');
        var when_array = [];
        var who_array = [];
        var branchnames = [];

        if (branches && branches.values[value.changeset_id])
        {
            for (var n in branches.values[value.changeset_id])
            {
                if (branches.closed[n])
                    branchnames.push('(' + n + ')');
                else
                    branchnames.push(n);
            }
        }

        $(value.audits).each(function (i, ts)
        {
            var d = new Date(parseInt(ts.timestamp));
            when_array.push(shortDateTime(d));
            var who = ts.username || ts.userid;
            who_array.push(who);
        });

        tdwhen.text(when_array.join(" ")).css("white-space", "nowrap");
        tdwho.text(who_array.join(" "));

        if (value.comments && value.comments.length > 0)
        {
            // history is turned off right now which messes this up
            // so check to see if we have history
            if (value.comments[0].history)
            {
                value.comments.sort(function (a, b)
                {
                    return (firstHistory(a).audits[0].timestamp - firstHistory(b).audits[0].timestamp);
                });
            }
            // var spancomment =
            // $('<span>').text(value.comments[0].text).addClass("comment");
            var vc = new vvComment(value.comments[0].text);

            tdcomment.html(vc.summary);
        }

        var tsSpans = [];
        if (value.tags != null)
        {
            var moreTags = $('<span class="moretags">');
            moreTags.hide();

            var hasMore = false;
            $(value.tags).each(function (i, v)
            {
                var span = $('<span>').text(v).addClass("vvtag");
                if (i < 2)
                {
                    tsSpans.push(span);
                }
                else
                {
                    hasMore = true;
                    moreTags.append(' ');
                    moreTags.append(span);
                }
            });

            tsSpans.push(moreTags);
            if (hasMore)
            {
                var moreTagsShowing = false;
                var aRevelationButton = $("<a>").css({ "width": "25px", "height": "15px", cursor: "default" }).addClass("tagExpander open");
                aRevelationButton.attr("title", "show more tags");

                aRevelationButton.click(function (e)
                {
                    e.preventDefault();
                    e.stopPropagation();
                    moreTagsShowing = !moreTagsShowing;
                    if (moreTagsShowing)
                    {
                        $(this).removeClass("open");
                        $(this).addClass("close");
                        $(this).attr("title", "show less tags");
                        moreTags.show();
                    }
                    else
                    {
                        $(this).removeClass("close");
                        $(this).addClass("open");
                        $(this).attr("title", "show more tags");
                        moreTags.hide();
                    }
                    hg.draw();

                });

            }
            tsSpans.push(aRevelationButton);
        }
        if (value.stamps != null)
        {
            $(value.stamps).each(function (i, v)
            {
                var a = $('<a>').attr(
							{
							    "title": "view changesets with this stamp",
							    href: sgCurrentRepoUrl + "/history.html?stamp=" + encodeURIComponent(v)
							});
                a.text(v);
                var span = $('<span>').append(a).addClass("stamp");
                tsSpans.push(span);

            });
        }

        $.each(tsSpans, function (i, span)
        {
            tdt.append(span);
        });
        var urlBase = window.location.pathname.rtrim("/")
							.rtrim("history.html").rtrim("/");

        tdlink.append(makePrettyRevIdLink(value.revno,
							value.changeset_id, true, null, null, urlBase));

        var tdgraph = $('<td>')
        var divgraphspace = $('<div>').addClass("graphspace");
        tdgraph.append(divgraphspace);
        

        // Note the .after method of append data doesn't work right in IE
        // var tdatas = tdt.after(tdlink).after(tdcomment).after(tds).after(tdwhen).after(tdwho);

        tr.append(tdlink);
        tr.append(tdgraph);
        tr.append(tdcomment);
        tr.append(tdt);
        tr.append(tdwhen);
        tr.append(tdwho);

        $('#tablehistory').append(tr);

        hg.addToGraph(value, tdcomment, $('#tablehistory tr').length - 1, branchnames);
		
		if (value.pseudo_children)
		{
			hg.connectPseudoChildren(value.changeset_id, value.pseudo_children);
		}
    });

    earliestRevShown = historyItems[historyItems.length - 1].revno;


    $('#tbl_footer').removeClass('spinning');

    // Need to account for the addition of the scrollbar after
    // the history table had been loaded
    vCore.refreshLayout();

    hg.draw();
    if (lastCSIDShown)
    {
        var tr = $("#" + lastCSIDShown);
        var pos = tr.offset().top - (tr.height() - 130);

        $(window).scrollTop(pos - $(window).height());
    }
    $("table.datatable tbody tr").hover(function ()
    {
        $(this).css("background-color", "#E4F0CE");
        //tdGraph.css("background-color", "white");
    }, function ()
    {
        $(this).css("background-color", "white");
    });
    loadingHistory = false;

    lastCSIDShown = historyItems[historyItems.length - 1].changeset_id;
}

function getBranches(historyItems)
{
    $('#tbl_footer .statusText').text("Loading Branches...");
    $('#tbl_footer').show();

    var url = sgCurrentRepoUrl + "/branches.json";
    vCore.ajax({
        url: url,
        dataType: 'json',

        error: function (xhr, textStatus, errorThrown)
        {
            $('#tbl_footer').removeClass('spinning');
        },
        success: function (data, status, xhr)
        {
            $('#tbl_footer .statusText').text("");
            $('#tbl_footer').hide();
            branches = data;
            if (!branchesLoaded)
            {
                var option = $('<option>').text("all").val(0);
                $("#selectBranch").append(option);
                var selectedOp = $.deparam.fragment()["branch"];

                $.each(branches.branches, function (i, v)
                {
                    if (!branches.closed[i])
                    {
                        var option = $('<option>').text(i).val(i);
                        $("#selectBranch").append(option);
                    }
                });
                var br = $.deparam.fragment()["branch"];
                if (br)
                    $("#selectBranch").val(br);
                branchesLoaded = true;
            }
        },
        complete: function ()
        {
            loadHistory(historyItems);
        }
    });
}

function getHistory(maxRows, fetchMore)
{
    $('#popup').hide();
    loadingHistory = true;
    var more = fetchMore || false;

    $('#tbl_footer .statusText').text("Loading History...");
    $('#tbl_footer').show();

    var str = "";

    //stamp is the only th that should come it through query string
    var tmpState = $.deparam.querystring();
    $.each(tmpState, function (i, v)
    {
        state[i] = v;
    });
    tmpState = $.deparam.fragment();

    $.each(tmpState, function (i, v)
    {
        state[i] = v;
    });
    if (maxRows || maxRows == 0)
        state["max"] = maxRows;

    if (fetchMore)
    {
        state["more"] = true;
    }

    setFlags();

    var url = sgCurrentRepoUrl + "/history.json?tok=" + sgPageToken;

    //merge the url and query string with the hash
    //string to create new query string
    var hurl = $.param.querystring(url, state, 0);

    var hasError = false;
    vCore.ajax({
        url: hurl,
        dataType: 'json',

        error: function (xhr, textStatus, errorThrown)
        {
            loadingHistory = false;
            $('#tbl_footer').removeClass('spinning');
        },
        success: function (data, status, xhr)
        {
            $('#tbl_footer .statusText').text("");
            $('#tbl_footer').hide();

            if (data.error)
            {
                loadingHistory = false;
                reportError(data.error);
                allRetrieved = true;
                hasError = true;
                return;
            }
            if (status != "success" || !data)
            {
                loadingHistory = false;
                return;
            }
            if (!data.more)
            {
                allRetrieved = true;
            }
            historyItems = data.items;
        },
        complete: function ()
        {
            if (!hasError)
            {
                if (!more)
                {
                    getBranches(historyItems);
                }
                else
                {
                    loadHistory(historyItems);
                }
            }
        }
    });

}

function populateBranchList(showall, selectDefault)
{
    _inputLoading($("#selectBranch"), true);
    var burl = sgCurrentRepoUrl + "/branch-names.json";
    if (showall)
        burl += "?all";

    vCore.ajax({
        url: burl,
        dataType: 'json',

        success: function (data)
        {
            $("#selectBranch").find('option').remove();

            $.each(data, function (i, v)
            {
                var option = $('<option>').text(v).val(v);
                $("#selectBranch").prepend(option);
            });
            var option = $('<option>').text("all").val(0);
            $("#selectBranch").prepend(option);
            if (selectDefault)
            {
                $("#selectBranch").val(0);
            }
            else
            {
                var br = $.deparam.fragment()["branch"];
                if (br)
                    $("#selectBranch").val(br);
            }
            branchesLoaded = true;
            _stopInputLoading($("#selectBranch"));
        },
        error: function ()
        {
            _stopInputLoading($("#selectBranch"));
        }

    });
}


function sgKickoff()
{
    $('#backgroundPopup').remove();
    $(window).scrollTop(0);

    $(window).unbind("resize");
  
    $(window).bind("resize", function ()
    {
        hg.onWindowResize();

        //        $("#h2History").css({ "min-width": "" + ($("#tablehistory").width() - 20) + "px" } );
    });

    $("#activity").bind("repaintGraph", function ()
    {
       // hg.draw();
    });
    scrollPosition = 0;

    $('#datefrom').datepicker({ dateFormat: 'yy/mm/dd', changeYear: true, changeMonth: true });
    $('#dateto').datepicker({ dateFormat: 'yy/mm/dd', changeYear: true, changeMonth: true });
    $(window).bind('hashchange', hashStateChanged);

    var a = $('#toggle_filter');
    a.toggle(function ()
    {
        $('#imgtoggle').attr("src", sgStaticLinkRoot + "/img/contract.png");
        $('#tableHistoryFilter').show();
        if (!stampsLoaded)
        {
            getAllStamps();
        }

        if (!usersLoaded)
        {
            getUsers();
        }

        $("#h2History").css({ "min-width": $("#tablehistory").width() });

        vCore.refreshLayout();
        hg.draw();
    },
	function ()
	{
	    $('#imgtoggle').attr("src", sgStaticLinkRoot + "/img/expand.png");
	    $('#tableHistoryFilter').hide();

	    hg.draw();
	});

    var hashString = $.param.fragment();
    if (hashString && hashString.length > 0)
    {
        setPageFromState();
    }

    getHistory(maxRows);

    $("#cbxShowAllBranches").click(function (e)
    {
        noUpdate = true;
        var all = $(this).is(':checked');
        state["allbranches"] = all;
        $.bbq.pushState(state);
        populateBranchList(all, true);

    });


    $("#cbxShowAllUsers").click(function (e)
    {
        noUpdate = true;
        var all = $(this).is(':checked');
        state["inactive"] = all;
        $.bbq.pushState(state);

        showInactiveUsers(all);
    }

    );

    $("#txtPath").keyup(function (e)
    {
        if ($(this).val() && $(this).val().length)
        {
            $("#cbxHideMerges").removeAttr("disabled");
            $("#labelHideMerges").removeClass("disabled");
        }
        else
        {
            $("#cbxHideMerges").attr("disabled", true);
            $("#labelHideMerges").addClass("disabled");
        }
    });
    $(window).scroll(function ()
    {
        // Chrome 5.0.x will call this twice:
        // http://code.google.com/p/chromium/issues/detail?id=43958
        // The loadingHistory flag is here to work around that.

        if (!loadingHistory && isScrollBottom() && !allRetrieved)
        {
            more = true;
            getHistory(maxRows, true);
        }
    });
}


