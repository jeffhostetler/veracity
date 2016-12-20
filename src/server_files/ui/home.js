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


function sortChangesetsByLastMod(inputHash)
{
    var aTemp = [];
    $.each(inputHash, function (key, val){
        aTemp.push(val);
    });

    aTemp.sort(function (a, b)
    {
        if (!a.audits)
            return -1;
        if (!b.audits)
            return 1;
        return (b.audits[0].timestamp - a.audits[0].timestamp);
    });

    return aTemp;
}

function setUpLinks()
{
    var error = false;
    vCore.ajax({
        url: sgCurrentRepoUrl + ".json?details",
        dataType: 'json',

        error: function (xhr, textStatus, errorThrown)
        {
            $('#recent').empty();
            if (xhr.statusText.toLowerCase() == "not found")
            {
                deleteFromRepoMenu(startRepo, function () { window.location.href = sgLinkRoot + "/repos.html" });
            }
            error = true;
        },
        success: function (data, status, xhr)
        {
            var countAnonLeaves = 0;
            var countClosed = 0;

            $('#recent').empty();

            if ((status != "success") || (!data))
            {
                return;
            }
            //            $('#recent').append($("<p>"));
            var hids = sortChangesetsByLastMod(data.hids);
            $.each(hids, function (idx, hid)
            {
            	var i = hid.changeset_id;
                var p = $('<div>').addClass("contents innercontainer");

                var span = $('<span>');

                if (hid.revno)
                {
                    var achange = $('<a>').attr({
                        'title': 'view most recent changeset ' + i,
                        href: sgCurrentRepoUrl + "/changeset.html?recid=" + i
                    }).text(hid.revno + ":" + i.slice(0, 10))
							.addClass("revno");

                    var zip = $('<a>').attr({
                        'title': 'download zip file for ' + i,
                        href: sgCurrentRepoUrl + "/zip/" + i + ".zip"
                    }).text("zip").addClass("small_link");
                    var browse = $('<a>').attr({
                        'title': 'browse changeset ' + i,
                        href: sgCurrentRepoUrl + "/browsecs.html?recid=" + i + "&revno=" + hid.revno
                    }).text("browse").addClass("small_link");
                    span.append(achange).append(zip).append(browse);


                    span.append($("<br>"));
                    var spants = $("<span>");
                    setTimeStamps(hid.audits, spants);
                    span.append(spants);
                }
                else
                {
                    span.text(hid.changeset_id.substr(0, 10) + " (not present in repository)");
                }

                if (hid.comments && hid.comments.length > 0)
                {
                    var div = formatComments(hid.comments);
                    span.append(div);
                }
                p.append(span);

                if (data.branches.values[hid.changeset_id])
                {
                    /* The changeset belongs to at least one named branch. */
                    var openBranchNames = [];
                    var closedBranchNames = [];
                    for (var n in data.branches.values[hid.changeset_id])
                    {
                        /* A single changeset could belong to both an open and a closed branch. */
                        if (data.branches.closed[n])
                            closedBranchNames.push(n);
                        else
                            openBranchNames.push(n);
                    }

                    if (openBranchNames.length > 0)
                    {
                        var spanOpenBranchNames = _safeBranchList(openBranchNames);
                        $('#recent').append(spanOpenBranchNames);
                        $('#recent').append(p);
                        $('#recent').append($("<p>"));
                    }
                    if (closedBranchNames.length > 0)
                    {
                        countClosed++;

                        if (openBranchNames.length > 0)
                            p = p.clone(true);

                        var spanClosedBranchNames = _safeBranchList(closedBranchNames);
                        $('#closed_branches').append(spanClosedBranchNames);
                        $('#closed_branches').append(p);
                        $('#closed_branches').append($("<p>"));
                    }
                }
                else
                {
                    countAnonLeaves++;
                    $('#un_named').append(p);
                    $('#un_named').append($("<p>"));

                }
            });
            if (countAnonLeaves > 0)
                $("#showunnamed").show();
            if (countClosed > 0)
                $("#showclosed").show();
        }
    });
    return !error;
}

function setRepoSelect(repos)
{
    var currentNames = [];
    var select = $('#reposelection').change(function ()
    {
        document.location.href = $(this).val();
    });

	$.each(repos, function (i, v)
	{
	    var url = sgLinkRoot + "/repos/" + encodeURIComponent(v);
	    var option = $('<option>').text(v).val(url).attr("id", i);
	    currentNames.push(v);
	    select.append(option);
	    if (v == startRepo)
	    {
	        select.val(url);
	    }

    });

    if (repos.length == 1)
    {
        var oneRepo = $('<b></b>').text(repos[0]);
        $('#pChooseRepo h3').text('Repository');
        $('#pChooseRepo div').prepend(oneRepo);
        select.hide();
        $('#pChooseRepo').removeClass('editable');
    }

    _stopInputLoading($('#reposelection'));
}

function _tryEmail(em, select)
{
    if (select)
        select.find('option').each(
			function (i, opt)
			{
			    if ($(opt).text() == em)
			    {
			        var uid = $(opt).val();
			        select.val(uid);
			        _saveUser(uid, em);
			    }
			}
		);
}


function usersFetched(data)
{
    var select = $('#userselection');

    $(document.createElement('option')).
	  val('').
	  text('Select a user ID to continue').
	  appendTo(select);

    data.sort(function (a, b) { return strcmp(a.name, b.name) });

    $.each(data, function (i, v)
    {
        var option = $('<option></option>').text(v.name).val(v.recid).attr({
            id: v.recid
        });

        select.append(option);

        if (v.recid == sgUserName)
        {
            option.attr('selected', 'selected');
        }
    });

    _stopInputLoading(select);

    // if the repo changed, the selected user email may still be valid, but
    // the associated ID will be different
    //
    if (!select.val())
    {
        var em = _savedEmail();

        if (em)
            _tryEmail(em);
    }

    if (!select.val())
    {
        var opt = select.find('option')[0];

        if (opt)
            _tryEmail($(opt).text());
    }

    if (select.val() && !sgUserName)
        _saveUser(select.val(), select.find("option:selected").text());

    if (select.val() && sgCurrentRepoUrl)
    {
        var img = $(document.createElement('img')).
			attr('src', sgStaticLinkRoot + "/img/blue_rss.png");

        var actXml = vCore.getOption('useractfeed') || (sgCurrentRepoUrl_universal + "/activity/" + select.val() + ".xml");

        var a = $(document.createElement('a')).
			attr('href', actXml).
			attr('title', 'Activity feed for this user').
			append(img);

        select.after(a);
    }
}


function sgKickoff()
{
    if (!setUpLinks())
        return;
    _inputLoading($("#userselection"));
    vCore.fetchUsers({ success: usersFetched });

    _inputLoading($('#reposelection'));
    vCore.fetchRepos({ success: setRepoSelect });

    $("#toggle_closed").toggle(
    function ()
    {
        $("#imgclosedtoggle").attr("src", sgStaticLinkRoot + "/img/contract.png");

        $("#closed_branches").show();
    },
    function ()
    {
        $("#imgclosedtoggle").attr("src", sgStaticLinkRoot + "/img/expand.png");
        $("#closed_branches").hide();
    });

    $("#toggle_unnamed").toggle(
    function ()
    {
        $("#imgunnamedtoggle").attr("src", sgStaticLinkRoot + "/img/contract.png");

        $("#un_named").show();
    },
    function ()
    {
        $("#imgunnamedtoggle").attr("src", sgStaticLinkRoot + "/img/expand.png");
        $("#un_named").hide();
    });

    $("#divhomecontent").show();
    $(".currentrepourl").text(sgCurrentRepoUrl_universal);
}

function _safeBranchList(branchnames)
{
    var h = $("<h2 class='innerheading'></h2>");

    for (var i = 0; i < branchnames.length; ++i)
    {
        if (i > 0)
        {
            h.append("<br />")
        }
        var l = $('<a href="' + branchUrl(branchnames[i]) + '"></a>').text(branchnames[i]);
        h.append(l);
    }

    return (h);
}
