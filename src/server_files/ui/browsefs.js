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

// TODO 2012/01/16 This code is based upon the PendingTree 
// TODO            version of STATUS and needs to be converted
// TODO            to the WC version.

var statuses = {};
var folderControl = {};
var fv = null;

function _lastSeg(url)
{
	var urlre = /.+\/(.+)\/?$/;

	var matches = urlre.exec(url);

	if (matches)
		return (matches[1]);
	else
		return (url);
}
function _normalizePath(url)
{
       var parts = url.split('/');
       var checkagain = true;

       while (checkagain)
       {
               checkagain = false;

               for ( var i = 1; i < parts.length; ++i)
               {
                       if (parts[i] == "..")
                       {
                               parts.splice(i - 1, 2);
                               checkagain = true;
                               break;
                       }
               }
       }

       var res = parts.join('/');

       return (res.replace(/\/\//, '/').replace(/\/$/, ''));
}

function status(path, id, stat)
{
	this.repPath = path;
	this.hid = id;
	this.status = stat;
}

function bindObservers()
{
	if (!isTouchDevice())
	{
		$(".files li").each(function(index)
		{
			$(this).find("span.linkSpan").first().hide()
			$(this).hover(function()
			{
				$(this).find("span.linkSpan").first().show();
			}, function()
			{
				$(this).find("span.linkSpan").first().hide();
			});
		});
	}
}

function _getStatuses()
{

    vCore.ajax(
	{
	    'url': '/local/status.json',
	    dataType: 'json',

	    success: function (data)
	    {
	        statuses = {};

	        for (var stat in data.status)
	        {
	            var files = data.status[stat];
               
	            for (var i in files)
	            {
	                var fn = files[i].path.replace(/^@/, '');
	                fn = fn.ltrim("/");
	                statuses[fn] = new status(files[i].path, files[i].old_hid,
							stat);

	            }

	        }

	    }

	});

}

function _applyStatus(path, file, top)
{
    var a = $('<a>').text(file).attr("href", "#").click(function (e)
    {
        var tmpPath = path.ltrim("@/");
     
        fv.displayLocalFile(tmpPath);    
    
    });

	var li = $('<li>').append(a);

	if (statuses[path])
	{

		if (statuses[path].status == "Modified")
		{
			var linkSpan = $("<span>").addClass("linkSpan");
			var diffURL = sgCurrentRepoUrl + "/diff_local.json?hid1=" + statuses[path].hid + "&path="
					+ statuses[path].repPath;
			var img = $('<img>').attr("src", sgStaticLinkRoot + "/img/diff.png");
			var a_diff = $('<a>').html(img).attr(
			{
				href : "#",
				'title' : "diff against baseline"
			}).addClass("download_link").click(function(e)
			{
				e.preventDefault();
				e.stopPropagation();
				showDiff(diffURL, $('<div>'), true);
			});
			linkSpan.append(a_diff);
			li.append(linkSpan);

		}
		li.addClass(statuses[path].status);
	}
	return li;
}

function _makeId(childurl)
{
	var idre = /[^a-zA-Z0-9]/g;

	var url = childurl.replace(/\/[^\/]*\/?$/, '');

	var theid = url.replace(idre, "_");

	return (theid);
}

function loadDir(url, containerraw)
{
    var container = $(containerraw);

    var path = getQueryStringVal("path", url);
	
    var progress = new smallProgressIndicator("folderLoading", "Loading Folder...");
    container.append(progress.draw());
	var ulFiles = $(document.createElement("ul")).addClass("files innercontainer");
	var ulFolders = $(document.createElement("ul")).addClass("folders primarydetails");

	vCore.ajax(
	{
	    'url': url,
	    dataType: 'json',

	    error: function (xhr, textStatus, errorThrown)
	    {
	        progress.hide();
	    },

	    success: function (data)
	    {
	        progress.hide();
	     
	        if (!data)
	        {
	            return;
	        }

	        container.empty();

			var h2 = $('<h2 class="innerheading"></h2>');
	        var dn = "@";

	        if (path)
	        {
	            var dn = _lastSeg(path);

	        }
	        h2.text(dn);
	        document.title = "Veracity / Browsing " + dn;
	        container.append(h2);

	        data.files.sort(function (a, b)
	        {
	            return (a.name.toLowerCase().compare(b.name.toLowerCase()));
	        });
	        $.each(data.files, function (index, entry)
	        {
	            var entryPath = entry.name;
	            if (path)
	                entryPath = path + "/" + entry.name;

	            var url = sgLinkRoot + "/local/fs.json";
	            if (entry.isDirectory)
	            {
	                var li = $(document.createElement("li"));

	                var curl = url + "?path=" + entryPath;

	                var cid = _makeId(_normalizePath(curl));
	                var a = folderControl.creatFolderDivLink(container,
							entry.name, cid, curl, loadDir, true);

	                li.append(a);
	                ulFolders.append(li);
	            }
	            else
	            {
	                var li = _applyStatus(entryPath, entry.name, data.cwd );

	                ulFiles.append(li);
	            }
	        });

	        if (path)
	        {
	            var li = $('<li>');
	            var parent = getParentPath(path);
	           
	            var curl = sgLinkRoot + "/local/fs.json?path=" + parent;
	            var cid = _makeId(_normalizePath(curl));
	            if (!parent || parent == "@")
	            {
	                curl = sgLinkRoot + "/local/fs.json";
	                cid = "ctr1";
	            }

	            a = folderControl.creatFolderDivLink(container, "..", cid, curl, loadDir, true);
	            li.append(a);
	            ulFolders.prepend(li);
	        }
	        //TODO handle missing/lost files

	        container.append(ulFolders);
	        container.append(ulFiles);
	        $('div.dir').removeClass('selected');
	        container.addClass('selected');
	        bindObservers();
	    }
	});

}

function sgKickoff()
{
	var initurl = sgLinkRoot + "/local/fs.json";
	_getStatuses();
	fv = new fileViewer();
	folderControl = new browseFolderControl();
	loadDir(initurl, $('#ctr1')[0]);
}
