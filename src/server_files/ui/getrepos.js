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

var repoDialog = null;
var selectRepo = null;
var selectRepoUsers = null;
var txtRepoName = null;
var getRepos_currentRepos = [];
var repos = {};
var addRepoUrl = "/repos.json";
var showShareUsers = true;
var repoRefreshTimer = null;
var timerNeeded = false;
var REPO_STATUS_NORMAL = 0;
var REPO_STATUS_CLONING = 1;
var REPO_STATUS_NEED_MAP = 2;
var REPO_STATUS_NEED_IMPORTING = 3;
var dirty = false;
var currentRepoName = "";
var Repo_Status =
{
    0: "normal",
    1: "cloning",
    2: "map users",
    3: "importing"
};


function menuClicked(e)
{
   
    e.stopPropagation();
    e.preventDefault();
    var action = $(this).attr("action");
    var reponame = $("#repocontextmenu").data("recid");
    var trId = $("#repocontextmenu").data("tr_id");
    var status = $(this).attr("status");
    $("#labelCurrRepo").text(reponame);

    if (repoRefreshTimer)
        clearTimeout(repoRefreshTimer);

    if (action == "clone")
    {
        $("#selectCloneRepo").val(reponame);
        $("#label_name_exists").hide();
        $(".shareUsers").hide();
        repoDialog.vvdialog("vvopen");
        txtRepoName.focus();
        $("#repocontextmenu").hide();
    }
    else if (action == "rename")
    {
        $(".trReader").show();
        $(".trEdit").hide();  
        
        var trEdit = $("#" + trId).clone().addClass("trEdit");
        $("#" + trId).after(trEdit);
        $("#" + trId).hide();
        $("#labelRenameError").hide();
        var tdName = trEdit.find(".tdreponame");
        $("#textRenameRepo").val(reponame);
        $("#textRenameRepo").data("reponame", reponame);
        $("#imgButtonSaveRepo").data("reponame", reponame);
        tdName.html($("#divEditRepoName"));
        $("#divEditRepoName").show();
        $("#textRenameRepo").focus();
        $("#textRenameRepo").select();
        $("#repocontextmenu").hide();
		$("#" + trId +" .tdrepocontext a").hide();
        $("#textRenameRepo").unbind("change");
        $("#textRenameRepo").change(function ()
        {
            dirty = $(this).data("reponame") != $(this).val();
        });
     
    }
    else if (action == "setdefault")
    {
        utils_savingNow("Setting default repo...");
        vCore.ajax({
            'type': "POST",
            'url': '/defaultrepo.json',
            'data': JSON.stringify({ 'repo': reponame }),
            'complete': function ()
            {
                $("#repocontextmenu").hide();
            },
            'error': function ()
            {
                utils_doneSaving();
            },
            'success': function ()
            {
                utils_doneSaving();
                getRepositories();
            }
        });
    }
}

function _checkRepoStatus()
{
    //TODO when there are repo statuses check the actual status of the clone.
    //Until then just get repos periodically until we see the new
    //name in the list. This will give a visual clue when the clone
    //is done if they stay on the page, but not if they navigate away until
    //we have repo statuses.
    var url = "/repos/manage.json";
    var continue_checking = false;
    vCore.ajax({
        type: "GET",
        url: url,
        success: function (data, status, xhr)
        {
            repos = data;
            displayRepos(repos);
            var count = 0;

            for (var i in data.descriptors)
            {
                var oneRepo = "";
                if (data.descriptors[i].status == REPO_STATUS_NORMAL)
                {
                    oneRepo = i;
                }
                data.descriptors[i].status
                if (data.descriptors[i].status == REPO_STATUS_CLONING || data.descriptors[i].status == REPO_STATUS_NEED_IMPORTING)
                {
                    continue_checking = true;
                    timerNeeded = true;
                    break;
                }
                count++;
            }
            if (repoRefreshTimer)
                clearTimeout(repoRefreshTimer);
            if (continue_checking)
            {
                repoRefreshTimer = window.setTimeout(function () { _checkRepoStatus(name); }, 10000);
            }
            else
            {
                //if this was the first repo and it was previously not normal state, redirect
                //back here with that a repo selected so 
                //everything just behaves
                if (timerNeeded && count.length == 1)
                {
                    document.location.href = sgLinkRoot + "/repos/" + encodeURIComponent(oneRepo) + "/repos.html";
                }
                timerNeeded = false;

            }
        }
    });

}

function addRepoContextMenu(tr, safeName, value, index)
{

    var tdcontext = $("<td>").addClass("tdrepocontext");
    var spacer = $('<div>').addClass("spacer");
    tdcontext.append(spacer);

    if (value && value.defaultRepo)
        tdcontext.addClass("default");

    if (value && (value.status == REPO_STATUS_NORMAL || value.status == REPO_STATUS_NEED_MAP))
    {
        var span = $("<span>").addClass("linkSpan");
        if (!isTouchDevice())
            span.hide();
        var img = $(document.createElement('img')).attr('src', sgStaticLinkRoot + "/img/contextmenu.png");
        var imgEdit = $(document.createElement('img')).attr('src', sgStaticLinkRoot + "/img/edit.png");
        a = $("<a>").html(img).attr({ "href": "#", "title": "view context menu", "id": "context" + safeName }).addClass("small_link");

        span.append(a);

        tdcontext.append(span);

        a.click(function (e)
        {
            e.preventDefault();
            e.stopPropagation();

            var menu = $('#repocontextmenu');
	    
            if (value.defaultRepo || value.status == REPO_STATUS_NEED_MAP)
                menu.find("li[action=setdefault]").hide();
            else
                menu.find("li[action=setdefault]").show();

            if (value.status == REPO_STATUS_NEED_MAP)
            {
                menu.find("li[action=clone]").hide();           
            }
		  else
		  {
	 		 menu.find("li[action=clone]").show();          
		  }
		    
            showCtxMenu($(this).attr("id"), $("#repocontextmenu"), { "recid": index, "tr_id": safeName });
        });
    }
    tr.prepend(tdcontext);

}
function displayRepos(repos)
{
    var data = repos.descriptors;
    
    $('#tablerepos tbody tr').remove();
    $('#selectCloneRepo option').remove();
    $('#selectUsersRepo option').remove();
    selectRepo.append($('<option value="-1">(None - Empty Repository)</option>'));
    selectRepoUsers.append($('<option value="-1">(None - Don\'t Share Users)</option>'));
    var count = 0;
    if (data)
    {
        $.each(data, function (index, value)
        {
            getRepos_currentRepos.push(index);
            var urepo = encodeURIComponent(index);
            var safeName = "tr" + count;
            count++;
            var tr = $('<tr>').attr("id", safeName).addClass("trReader");
            if (value.status)
            {
                var td = $('<td>').addClass("tdreponame").text(index);
                var tdtags = $('<td>');
                var tdzip = $('<td>');
                tr.append(td);
                switch (value.status)
                {
                    case 1:
                    case 3:
                        {
                            timerNeeded = true;
                            tr.append('<td colspan="3" class="spinning"><img src="' + sgStaticLinkRoot + '/img/smallProgress.gif"/> ' + Repo_Status[value.status] + '...</td>');
                        }
                        break;
                    case 2:
                        {
                            var tdmap = $('<td colspan="3" class="spinning"><img src="' + sgStaticLinkRoot + '/img/builds/testfailed16.png"/></td>');
                            var a = $('<a class="linkmapusers" href="#"> ' + Repo_Status[value.status] + '</a>');
                            a.data("repoName", index);
                            tdmap.append(a);
                            tr.append(tdmap);
                        }
                        break;
                    default:
                        tr.append('<td colspan="3" >' + Repo_Status[value.status] + '</td>');
                        break;
                }
                $('#tablerepos tbody').append(tr);
                if (value.status == 1 || value.status == 3)
                {
                    if (repoRefreshTimer)
                        clearTimeout(repoRefreshTimer);
                    repoRefreshTimer = window.setTimeout(function () { _checkRepoStatus(index); }, 10000);
                }
            }
            else
            {
                var option = $('<option>').text(index).val(index);
                var option2 = $('<option>').text(index).val(index);
                selectRepo.append(option);
                selectRepoUsers.append(option2);
                tr = $('<tr>').attr("id", safeName).addClass("trReader");
                var td = $('<td>').addClass("tdreponame");
                var tdtags = $('<td>');
                var tdzip = $('<td>');
                var a = $('<a>').attr({
                    'title': 'view repository history',
                    href: sgLinkRoot + "/repos/" + urepo + "/history.html"
                }).text(index);
                td.append(a);
                tr.append(td);
                var atags = $('<a>').attr({
                    'title': 'view tags',
                    href: sgLinkRoot + "/repos/" + urepo + "/tags/"
                }).text("tags").addClass("small_link");
                tdtags.append(atags);
                var hids = value.hids;
                var branches = value.branches;
                if (hids.length > 1)
                {
                    var azip = $('<a>').attr({
                        'title': 'view repository leaves',
                        href: "#"
                    }).text("leaves");
                    tdzip.append(azip);
                    azip.click(function (e)
                    {
                        e.preventDefault();
                        e.stopPropagation();
                        var table = $('<table>');
                        $.each(hids, function (i, v)
                        {
                            var tr2 = $('<tr>');
                            var td1 = $('<td>');
                            var td2 = $('<td>');
                            var tdzip2 = $('<td>');
                            if (branches && branches.values[v])
                            {
                                var name = "";
                                for (var n in branches.values[v])
                                {
                                    name = n;
                                    if (branches.closed[n])
                                    {
                                        name = "(" + name + ")";
                                    }
                                }
                                td1.text(name);
                            }
                            tr2.append(td1);
                            var cslink = makePrettyRevIdLink(null, v, true, null, null, sgLinkRoot + "/repos/" + urepo);
                            td2.append(cslink);
                            var azip = $('<a>').attr({
                                'title': 'download zip file for ' + v,
                                href: sgLinkRoot + "/repos/" + urepo + "/zip/" + v + ".zip"
                            }).text("zip").addClass("small_link");
                            var achange = $('<a>').attr({
                                'title': 'view most recent changeset ' + v,
                                href: sgLinkRoot + "/repos/" + urepo + "/changeset.html?recid=" + v
                            }).text("changeset").addClass("small_link");
                            var abrowse = $('<a>').attr({
                                'title': 'browse changeset ' + v,
                                href: sgLinkRoot + "/repos/" + urepo + "/browsecs.html?recid=" + v
                            }).text("browse").addClass("small_link");
                            tdzip2.append(achange);
                            tdzip2.append(abrowse);
                            tr2.append(td2);
                            tr2.append(tdzip2);
                            table.append(tr2);
                        });
                        var pop = new modalPopup();
                        $('#maincontent').prepend(pop.create("leaves", table));
                        pop.show(null, null, tdzip.position());
                    });
                }
                else
                {
                    var azip = $('<a>').attr({
                        'title': 'download zip file for ' + hids[0],
                        href: sgLinkRoot + "/repos/" + urepo + "/zip/" + hids[0] + ".zip"
                    }).text("zip").addClass("small_link");
                    var achange = $('<a>').attr({
                        'title': 'view most recent changeset ' + hids[0],
                        href: sgLinkRoot + "/repos/" + urepo + "/changeset.html?recid=" + hids[0]
                    }).text("changeset").addClass("small_link");
                    var abrowse = $('<a>').attr({
                        'title': 'browse most recent changeset' + hids[0],
                        href: sgLinkRoot + "/repos/" + urepo + "/browsecs.html?recid=" + hids[0]
                    }).text("browse").addClass("small_link");
                    tdzip.append(azip);
                    tdzip.append(achange);
                    tdzip.append(abrowse);
                }
                tr.append(tdzip);
                tr.append(tdtags);
                $('#tablerepos tbody').append(tr);
            }
            addRepoContextMenu(tr, safeName, value, index);
        });
        if (repos.has_excludes)
        {
            $(".tdrepocontext").hide();
        }
       /* $('table.datatable tr').hover(function ()
        {
            $(this).css("background-color", "#E4F0CE");
            if (!isTouchDevice() && $(".tdrepocontext").is(":visible"))
                $(this).find(".linkSpan").first().show();
        },function ()
        {
            $(this).css("background-color", "white");
            if (!isTouchDevice() && $(".tdrepocontext").is(":visible"))
                $(this).find(".linkSpan").first().hide();
        });*/
        enableRowHovers(repos.has_excludes);
        $(".ctxMenuItem").unbind("click");
        $(".ctxMenuItem").click(menuClicked);
    }
    if (count == 0)
    {
        var tr = $('<tr class="trReader"><td colspan="4" class="emptytable">There are no repositories available on this server. Click the New Repository link to add a repository</td></tr>');
        $('#tablerepos tbody').append(tr);
    }
    addRenameForm();
    utils_doneSaving();
    $(window).trigger("getRepos_Done");
}

function addRenameForm()
{
    $("#textRenameRepo").unbind("keydown");
    $("#imgButtonSaveRepo").unbind("click");
    $("#imgButtonCancelRepo").unbind("click");
    var form = $('<div id="divEditRepoName" style="display:none"> ' +'<input type="text" id="textRenameRepo" maxlength="255" size="50"/>' +'<input type="image" id="imgButtonSaveRepo" src="' + sgStaticLinkRoot + '/img/save16.png" />' +'<input type="image" id="imgButtonCancelRepo" src="' + sgStaticLinkRoot + '/img/cancel18.png" /><label id="labelRenameError" class="rename-warning" style="display:none"> name already exists</label>' +'<div id="divRenameWarning" > <p>' +'Renaming a repository breaks all associations with current working copies of that repository. To create a working copy ' +'for the new name, perform a checkout.  </p>   <p>' +'Additionally, no config settings (such as the default push/pull location) will be migrated to the new name. Any desired ' +'repository-specific settings must be set manually under the new name. To see the old settings, run:<br />' +'vv config show /instance/<label id="labelCurrRepo"></label></p> </div></div>');
    $("#maincontent").append(form);
    $("#textRenameRepo").keydown(function (e)
    {
        if (e.which == 13)
        {
            e.preventDefault();
            e.stopPropagation();
            renameRepository($(this).data("reponame"), $(this).val());
        }
        else if (e.which == 27)
        {
            $("#divEditRepoName").hide();
            $(".trReader").show();
            $(".trEdit").hide();
			$(".tdrepocontext a").show();
            $(this).val("");
            startTimerIfNeeded();
            dirty = false;
            currentRepoName = "";
        }
    });
    $("#imgButtonSaveRepo").click(function (e)
    {
        e.preventDefault();
        e.stopPropagation();
        renameRepository($(this).data("reponame"), $("#textRenameRepo").val());
    });
    $("#imgButtonCancelRepo").click(function (e)
    {
        $("#divEditRepoName").hide();
        $(".trReader").show();
        $(".trEdit").hide();
		$(".tdrepocontext a").show();
        $(this).val("");
        startTimerIfNeeded();
        dirty = false;

    });
}

function _isNameAvailable(name)
{
    if ($.inArray(name, getRepos_currentRepos) >= 0)
    {
        return false;
    }

    return true;

}

function addRepository(name, shareUserRepo)
{
    var url = addRepoUrl;
    utils_savingNow("Saving repository...");
    var count = getRepos_currentRepos.length;
    var json = { "name": name };
    if (shareUserRepo)
    {
        json.shareUsers = shareUserRepo;
    }
    vCore.ajax(
	{
	    type: "POST",
	    url: url,
	    data: JSON.stringify(json),
	    success: function (data, status, xhr)
	    {
	        //if this was the first repo, redirect
	        //back here with that a repo selected so 
	        //everything just behaves
	        if (count == 0)
	        {
	            document.location.href = sgLinkRoot + "/repos/" + encodeURIComponent(name) + "/repos.html";
	        }
	        else
	        {
	            getRepositories();
	            _displayLastVisitedRepos();
	        }
	    },
	    complete: function ()
	    {
	        resetRepoForm();
	    }
	});

}

function cloneRepository(name, newname)
{
    var url = "/repos/" + encodeURIComponent(name) + "/clone.json";
    utils_savingNow("Cloning repository. This can be a lengthy process...");
    vCore.ajax(
	{
	    type: "POST",
	    url: url,
	    data: JSON.stringify({ "name": newname }),
	    success: function (data, status, xhr)
	    {
	        repos.descriptors[newname] = { status: REPO_STATUS_CLONING };
	        displayRepos(repos);

	        utils_doneSaving();
	        _displayLastVisitedRepos();
	    },
	    complete: resetRepoForm

	});
}

function renameRepository(oldName, newName)
{
    newName = newName.trim();
    var error = _validateRepoName(newName);
    if (error)
    {
        $("#labelRenameError").text(error);
        $("#labelRenameError").show();
        return;
    }
    else
    {
        $("#labelRenameError").hide();
    }

    var url = "/repos/manage/" + encodeURIComponent(oldName) + ".json";
    utils_savingNow("Saving repository...");
    vCore.ajax(
	{
	    type: "PUT",
	    url: url,
	    data: JSON.stringify({ "name": newName }),
	    success: function (data, status, xhr)
	    {
			$(".tdrepocontext a").show();
	        getRepositories();
	        dirty = false;
	    }
	});

}

function _submitRepoForm()
{
    var name = $("#txtRepoName").val().trim();
    var error = _validateRepoName(name);
    if (error)
    {
        $("#label_name_exists").text(error);
        $("#label_name_exists").show();
        return;
    }
    else
    {
        $("#label_name_exists").hide();
    }

    var selected = $("#selectCloneRepo option:selected").val();
    if (selected == "-1")
    {
        var shareUserRepo = null;
        var val = $("#selectUsersRepo option:selected").val();
        if (val != "-1")
            shareUserRepo = val;

        addRepository(name, shareUserRepo);
    }
    else
    {
        cloneRepository(selected, name);
    }
    repoDialog.vvdialog("vvclose");


    //TODO else show error

}

function _validateRepoName(name)
{
    if (!name)
        return "repository name required";
    if (!_isNameAvailable(name))
    {
        return "name already exists";
    }
    var error = validate_object_name(name, get_object_name_constraints());
    if (error)
    {
        return error;
    }

    return "";
}

function startTimerIfNeeded()
{
    if (timerNeeded)
        _checkRepoStatus();    
}

function resetRepoForm()
{
    $("#txtRepoName").val("");
    $("#selectCloneRepo").val("-1");
    dirty = false;
    currentRepoName = "";
}

function getRepositories()
{
    getRepos_currentRepos = [];
    var url = "/repos/manage.json";
    utils_savingNow("Getting repositories...");
    vCore.ajax(
	{
	    type: "GET",
	    url: url,

	    success: function (data, status, xhr)
	    {
	        repos = data;
	        displayRepos(data);
	    }

	});

}

function sgKickoff()
{
    selectRepo = $('<select id="selectCloneRepo"></select>');
    selectRepoUsers = $('<select class="shareUsers" id="selectUsersRepo"></select>');

    selectRepo.change(function ()
    {
        if ($(this).val() == "-1")
        {
            $(".shareUsers").show();
        }
        else
        {
            $(".shareUsers").hide();
        }
    });
    getRepositories();
    repoDialog = $('<div class="popupform primary editable"></div>')
		.vvdialog({
		    modal: true,
		    resizable: false,
		    width: 400,
		    //height: 250,
		    minHeight: 250,
		    close: function (event, ui)
		    {
		        $("#backgroundPopup").hide();
		        resetRepoForm();
		    },
		    buttons:
			{
			    "Create Repository": function ()
			    {
			        _submitRepoForm();
			    },
			    "Cancel": function ()
			    {
			        $(this).vvdialog("vvclose");
			        // resetRepoForm();
			    }
			},
		    autoOpen: false,
		    title: "New Repository"

		});


		txtRepoName = $('<input type="text" id="txtRepoName" maxlength="255" class="innercontainer"></input>');
		
     repoDialog.append($('<label for="selectCloneRepo" class="innerheading blocklabel">Clone: </label>'));

    $('<div class="inputwrapper wrapblock"></div>').
		append(selectRepo).
		appendTo(repoDialog);

    if (showShareUsers)
    {
        var spanShareUsers = $('<div id="divShareUsers"></div>').addClass("shareUsers");
        repoDialog.append(spanShareUsers);
        spanShareUsers.append($('<label class="shareUsers innerheading blocklabel" for="selectUsersRepo">Share Users With: </label>'));

        $('<div class="inputwrapper wrapblock"></div>').
			append(selectRepoUsers).
			appendTo(spanShareUsers);
    }
    repoDialog.append($('<label for="txtRepoName" class="innerheading blocklabel">Name: </label>'));
    repoDialog.append(txtRepoName);
    repoDialog.append($('<label id="label_name_exists" class="rename-warning" style="display:none">The repository name already exists</label>'));
    txtRepoName.keydown(function (e)
    {
        if (e.which == 13)
        {
            e.preventDefault();
            e.stopPropagation();

            _submitRepoForm();
        }
    });


    $("#linknewrepo").click(function (e)
    {
        e.preventDefault();
        e.stopPropagation();
        if (repoRefreshTimer)
            clearTimeout(repoRefreshTimer);
        $("#label_name_exists").hide();
        $('.shareUsers').show();
        repoDialog.vvdialog("vvopen");
        txtRepoName.focus();

    });

 
    window.onbeforeunload = function ()
    {      
        if (dirty)
            return "You have unsaved changes.";
    }

    $("#divnewrepo").show();
}
