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


var hideIndex = 0;
var folderControl = {};
var _changesetHover = null;
var fv = null;

function displayBrowse(data, container) {

    container.attr({ id: data.path.rtrim("/") });
    var revno = getQueryStringVal("revno") || null;
    var a = makePrettyRevIdLink(revno, data.changeset_id, false);
    _changesetHover.addHover(a, data.changeset_id);
	a.children().removeClass("small_link");
    $('#csid').html(a);
    $('#csid').prepend("Revision: ");
    
    vCore.setPageTitle(data.name, "Browse Changeset");
    if (data.type == "File") {

        var parentUrl = sgCurrentRepoUrl + "/changesets/" + getQueryStringVal("recid") + "/browsecs.json?path=" + getParentPath(data.path);
        loadBrowse(parentUrl, $('#ctr1')[0]);
        fv.displayFile(data.hid, null, data.name);

    }
    else {
        var h2 = $('<h2 class="innerheading"></h2>').
			appendTo(container);

        var ulFiles = $('<ul>').addClass("files innercontainer");
        var ulFolders = $('<ul>').addClass("folders primarydetails"); ;

        h2.text(data.name);
        var a = $('<a>');

        data.contents.sort(function(a, b) {
            return (b.name.toLowerCase().compare(a.name.toLowerCase()));
        });
        $.each(data.contents, function (index, entry)
        {
            var isDir = (entry.type == "Directory");
            var li = $('<li>');
            var curl = sgCurrentRepoUrl + "/changesets/" + getQueryStringVal("recid") + "/browsecs.json?gid=" + entry.gid;
      
            if (isDir)
            {

                a = folderControl.creatFolderDivLink(container, entry.name, entry.path.rtrim("/"), curl, loadBrowse, false);
                li.prepend(a);
                ulFolders.prepend(li);
            }

            else
            {
                a = folderControl.creatFileDivLink(container, entry.hid, entry.name);
				var fn = lastURLSeg(entry.path);

                var downloadURL = sgCurrentRepoUrl + "/blobs-download/" + entry.hid + "/" + fn;

                var linkSpan = $("<span class='linkSpan'></span>").
					appendTo(li);
                var img = $('<img>').
					attr("src", sgStaticLinkRoot + "/img/green_download_Xsmall.png");
                var a_download = $('<a class="download_link"></a>').
					attr({ href: downloadURL, 'title': "download file" }).
					append(img).
					appendTo(linkSpan);

                if (sgLocalRepo)
                {
                    var diff_div = $('<div>');
                    var diffURL = sgCurrentRepoUrl + "/diff_local.json?hid1=" + entry.hid + "&path=" + entry.path;
                    var imgd = $('<img>').attr("src", sgStaticLinkRoot + "/img/diff.png");

                    var a_diff = $('<a>').html(imgd).attr({ href: "#", 'title': "diff against current working copy" })
			                                        .addClass("download_link").click(function (e)
			                                        {
			                                            e.preventDefault();
			                                            e.stopPropagation();
			                                            showDiff(diffURL, diff_div, true, "Diff: " + fn);

			                                        });
                    linkSpan.append(a_diff);
                }
                li.prepend(a);
                ulFiles.prepend(li);
            }

        });

        if (data.path.rtrim("/") != "@") {

            var li = $('<li>');
            var parent = getParentPath(data.path);
           
            var curl = sgCurrentRepoUrl + "/changesets/" + getQueryStringVal("recid") + "/browsecs.json?path=" + parent;
            a = folderControl.creatFolderDivLink(container, "..", parent, curl);
            li.append(a);
            ulFolders.prepend(li);
        }

        else {
            container.attr({ id: "@" });
        }

        container.append(ulFolders);
        container.append(ulFiles);
        $('div.dir').removeClass('selected');
        container.addClass('selected');
    }

}

function bindObservers() {
	if (!isTouchDevice())
	{
		$(".files li").each(function(index) {
			$(this).find("span.linkSpan").first().hide()
			$(this).hover(
				function() { $(this).find("span.linkSpan").first().show(); },
				function() { $(this).find("span.linkSpan").first().hide(); }
			);
		});
	}
}

function loadBrowse(url, containerraw) {

    var container = $(containerraw);
    

    var diff_div = $('<div>').addClass("cs_info");
    var progress = new smallProgressIndicator("folderLoading", "Loading Folder...");
    container.append(progress.draw());

    vCore.ajax({
        dataType: 'json',
        url: url,
        errorMessage: url,
        error: function (xhr, textStatus, errorThrown)
        {
            progress.hide();
        },
        success: function (data, status, xhr)
        {

            progress.hide();

            displayBrowse(data, container);
            bindObservers();
        }
    });

}

function sgKickoff() {

    var initurl = sgCurrentRepoUrl + "/changesets/" + getQueryStringVal("recid") + "/browsecs.json";
    var path = getQueryStringVal("path");
    var gid = getQueryStringVal("gid");
    _changesetHover = new hoverControl(fetchChangesetHover);
    fv = new fileViewer();
    if (path)
    {
        initurl += "?path=" + path;
    }
    if (gid)
    {
        initurl += "?gid=" + gid;
    }
   
    folderControl = new browseFolderControl();
    loadBrowse(initurl, $('#ctr1')[0]);
}
