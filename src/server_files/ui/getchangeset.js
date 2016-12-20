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

var _changesetHover = null;
var _wiHover = null;
var bubbleError = null;
var tagError = null;
var stampError = null;
var stamps = [];
var fv = null;
var changeset = null;
var gcscsid = null;

var mergeReviewPane = null;
var revnoToRecid = {};

var setContainingBranches = function(branches){
	$('#tdBranches').empty();
	
	if(branches.length==0){
		$('#tdBranches').text("(There are no open branches that contain this changeset.)");
	}
	else{
		for(var i=0; i<branches.length; ++i){
			var head = branches[i][0]=="!";
			var b = branches[i].slice(1);
			if(i>0){
				$('#tdBranches').append(", ");
			}
			var link = $('<a></a>').attr({title: b, href: branchUrl(b)}).text(b);
			if(head){
				link.css({"font-weight": "bold"});
			}
			$('#tdBranches').append(link);
		}
	}
	$("#trBranches").show();
}

function setComments(comments)
{
	if (comments && comments.length > 0)
	{
		$('#comment').empty();

        if (comments.length==1)
        {
            var c = new vvComment(comments[0].text);
            $("#tdComment").html(c.details);
            $("#trComment").show();
            $("#divMultipleComments").hide();
        }
        else
        {
            var cmt = formatComments(comments);
            $('#comment').append(cmt);
            $("#trComment").hide();
            $("#divMultipleComments").show();
        }

    }
}


function _wrappablePath(fn)
{
	var name = fn || '';

	return( name.replace(/\//g, "/\u200B") );
}

var populatedDiffViews = [];
var certainDiffToShow = false;
var changesetDetailsShowCertainDiff = function(parentRevnoToShow){
	if(parentRevnoToShow!='all'){
		certainDiffToShow = parentRevnoToShow;
		$('#mergereviewsection').show();
		$('#genericfilechangesheader').show();
		$('.mergesummary').hide();
		$.each(populatedDiffViews, function(index, revno){
			$("#title_parent_revno_"+revno).hide();
			if(revno==parentRevnoToShow){
				$("#diff_parent_revno_"+revno).show();
			}
			else{
				$("#diff_parent_revno_"+revno).hide();
			}
		});
	}
	else{
		$('#mergereviewsection').hide();
		$('#genericfilechangesheader').hide();
		$('.mergesummary').show();
		$.each(populatedDiffViews, function(index, revno){
			$("#diff_parent_revno_"+revno).show();
			$("#title_parent_revno_"+revno).show();
		});
	}
	
}

/*****************************************************************
 *****************************************************************

function showChangedFiles(diff, div, revno)
{
    var ul = $('<ul>').addClass("cslist");

    $.each(diff, function (index, value)
    {
        value.sort(function (a, b)
        {
            return (b.path.toLowerCase().compare(a.path.toLowerCase()));
        });
        $.each(value, function (ii, vv)
        {
            var isDirectory = (vv.type == "Directory");
            var li = $('<li>');
            var sp = $('<span>');
            var toggle = $('<span>');
            var attrs = " ";
            if (vv.changed_attr)
            {
                attrs += " (Attributes) ";
            }
            sp.text(index + attrs);
            var plus = $('<a>').text("+").attr(
			{
			    href: "#"
			}).addClass("plus");
            var info = $('<div>').addClass("cs_info");

            plus.toggle(function ()
            {
                plus.text("-");
                info.css(
				{
				    "padding-top": "5px",
				    "padding-bottom": "5px"
				});
                info.show();
            }, function ()
            {
                plus.text("+");
                info.hide();
            });

            toggle.append(plus);
            if (index != "Added" && index != "Removed" && index != "Modified")
            {
                li.append(toggle);
                li.addClass("toggle");
            }

            li.append(sp);

			var showPath = _wrappablePath(vv.path);

            if (isDirectory && index == "Removed")
            {
                li.text("Removed " + showPath);
            }
            else
            {
                var a = $('<a>').text(showPath);
                if (isDirectory)
                {
                    a.attr(
				        {
				            href: sgCurrentRepoUrl + "/browsecs.html?recid=" + gcscsid + "&gid=" + vv.gid + "&revno=" + revno,
				            'title': "display folder"
				        });

                }
                else
                {
                    a.attr(
				        {
				            href: "#",
				            'title': "display file"
				        }).click(function ()
				        {
				            fv.displayFile(vv.hid, showPath.ltrim("@/"));
				        });
                }

                li.append(a);
            }
            if (vv.from)
            {
                info.append(" moved from " + _wrappablePath(vv.from)).hide();
            }
            if (vv.oldname)
            {
                info.append(" original name " + vv.oldname).hide();
            }

            var linkSp = $("<span>").addClass("linkSpan");

            if (isDirectory == false && index != "Removed")
            {
                var downloadURL = sgCurrentRepoUrl + "/blobs-download/" + vv.hid + "/"
						+ lastURLSeg(vv.path);
                var img = $('<img>').attr("src",
						sgStaticLinkRoot + "/img/downloadgreen2.png");
                var a_download = $('<a>').html(img).attr(
				{
				    href: downloadURL,
				    'title': "download file"
				}).addClass("small_link");

                if (index != "Added" && index != "Moved" && index != "Renamed")
                {
                    var diff_div = $('<div>');
                    var diffURL = sgCurrentRepoUrl + "/diff.json?hid1=" + vv.old_hid + "&hid2=" + vv.hid + "&file="
							+ lastURLSeg(vv.path);
                    var imgd = $('<img>').attr("src",
							sgStaticLinkRoot + "/img/diff.png");

                    var a_diff = $('<a>').html(imgd).attr(
					{
					    href: "#",
					    'title': "diff against previous version"
					}).addClass("download_link").click(function (e)
					{
					    e.preventDefault();
					    e.stopPropagation();
					    showDiff(diffURL, diff_div, true);

					});
                    linkSp.append(a_diff);
                }

                linkSp.append(a_download);
            }

            li.append(linkSp);

            ul.prepend(li);
            li.append(info);
        });
    });
    if (div)
    {
        div.append(ul);
    }
    else
    {
        $('#changes').append(ul);
    }
}

*****************************************************************
*****************************************************************/

/*****************************************************************
 ** Show body of diff assuming canonical status info.
 *****************************************************************/

function fmt_display_path(item_i, cslabels, revno)
{
	var path = _wrappablePath(item_i.path);

	if (item_i.status.isRemoved)
	{
		var parens = "(" + path + ")";

		if (!item_i.status.isFile)
		{
			return parens;
		}
		else
		{
			// for a removed file, we offer to display the contents
			// of the old/left side.

			var a_display = $('<a>').text(parens);
			a_display.attr(
				{ href: "#",
				  'title': "display file"
				}).click(function ()
						 {
							 fv.displayFile(item_i[cslabels[0]].hid, path.ltrim("@/")); 
						 });

			return a_display;
		}
	}
	else
	{
		var a_display = $('<a>').text(path);
		if (item_i.status.isDirectory)
		{
			a_display.attr(
				{ href : sgCurrentRepoUrl + "/browsecs.html?recid=" + getQueryStringVal("recid") + "&gid=" + item_i.gid + "&revno=" + revno,
				  'title': "display folder"
				});
		}
		else if (item_i.status.isFile)
		{
			a_display.attr(
				{ href: "#",
				  'title': "display file"
				}).click(function ()
						 {
							 fv.displayFile(item_i[cslabels[1]].hid, path.ltrim("@/")); 
						 });
		}

		return a_display;
	}
}

function fmt_download_path(item_i, cslabels)
{
	if (!item_i.status.isFile)			// should assert !this
		return "";

	var downloadURL = sgCurrentRepoUrl + "/blobs-download/" + item_i[cslabels[1]].hid + "/" + lastURLSeg(item_i.path);
	var img = $('<img>').attr("src", sgStaticLinkRoot + "/img/downloadgreen2.png");
	var a_download = $('<a>').html(img).attr(
		{ href: downloadURL,
		  'title': "download file"
		}).addClass("small_link");

	var linkSp = $('<span>').addClass("linkSpan");
	linkSp.append( a_download );

	return linkSp;
}

function fmt_diff_path(item_i, cslabels)
{
	if (!item_i.status.isFile)			// should assert !this
		return "";

    var diff_div = $('<div>');
    var diffURL = sgCurrentRepoUrl + "/diff.json?hid1=" + item_i[cslabels[0]].hid + "&hid2=" + item_i[cslabels[1]].hid + "&file=" + lastURLSeg(item_i.path);
    var imgd = $('<img>').attr("src", sgStaticLinkRoot + "/img/diff.png");
    var a_diff = $('<a>').html(imgd).attr(
		{ href: "#",
		  'title': "diff against previous version"
		}).addClass("download_link").click(function (e)
										   {
											   e.preventDefault();
											   e.stopPropagation();
											   showDiff(diffURL, diff_div, true);
										   });

	var linkSp = $('<span>').addClass("linkSpan");
	linkSp.append( a_diff );

	return linkSp;
}

function fmt_added(item_i, cslabels, revno)
{
	var tr = $('<tr>');

	var td_section = $('<td>').text("Added");
	tr.append(td_section);

	var td_toggle = $('<td>');
	tr.append(td_toggle);

	var td_detail = $('<td>');
	td_detail.append( fmt_display_path(item_i, cslabels, revno)  );
	td_detail.append( fmt_download_path(item_i, cslabels) );

	tr.append(td_detail);
	return tr;
}

function fmt_modified(item_i, cslabels, revno)
{
	var tr = $('<tr>');

	var td_section = $('<td>').text("Modified");
	tr.append(td_section);

	var td_toggle = $('<td>');
	tr.append(td_toggle);

	var td_detail = $('<td>');
	td_detail.append( fmt_display_path(item_i, cslabels, revno)  );
	td_detail.append( fmt_diff_path(item_i, cslabels) );
	td_detail.append( fmt_download_path(item_i, cslabels) );

	tr.append(td_detail);

	return tr;
}

function fmt_attributes(item_i, cslabels, revno)
{
	var tr = $('<tr>');

	var td_section = $('<td>').text("Attributes");
	tr.append(td_section);

	var td_toggle = $('<td>');
	tr.append(td_toggle);

	var td_detail = $('<td>');
	td_detail.append( fmt_display_path(item_i, cslabels, revno)  );
	td_detail.append( fmt_download_path(item_i, cslabels) );

	tr.append(td_detail);

	return tr;
}

function fmt_removed(item_i, cslabels, revno)
{
	var tr = $('<tr>');

	var td_section = $('<td>').text("Removed");
	tr.append(td_section);

	var td_toggle = $('<td>');
	tr.append(td_toggle);

	var td_detail = $('<td>');
	td_detail.append( fmt_display_path(item_i, cslabels, revno)  );

	tr.append(td_detail);

	return tr;
}

function fmt_renamed(item_i, cslabels, revno)
{
	var tr = $('<tr>');

	var td_section = $('<td>').text("Renamed").attr("valign", "top");
	tr.append(td_section);

	var info = $('<div>').addClass("cs_info");
	info.append("Original name: " + item_i[cslabels[0]].name).hide();

	var plus = $('<a>').text("+").attr({href: "#"}).addClass("plus");
	plus.toggle(function ()
				{
					plus.text("-");
					info.css(
						{
							"padding-left": "20px",
							"padding-top": "5px",
							"padding-bottom": "5px"
						});
					info.show();
				}, 
				function ()
				{
					plus.text("+");
					info.hide();
				});

	var td_toggle = $('<td>').attr("valign", "top");
	td_toggle.append(plus);
	tr.append(td_toggle);

	var td_detail = $('<td>');
	td_detail.append( fmt_display_path(item_i, cslabels, revno)  );
	td_detail.append( fmt_download_path(item_i, cslabels) );
	td_detail.append( info );
	tr.append(td_detail);

	return tr;
}

function fmt_moved(item_i, cslabels, revno)
{
	var tr = $('<tr>');

	var td_section = $('<td>').text("Moved").attr("valign", "top");
	tr.append(td_section);

	var info = $('<div>').addClass("cs_info");
	info.append("Original path: " + item_i[cslabels[0]].path).hide();

	var plus = $('<a>').text("+").attr({href: "#"}).addClass("plus");
	plus.toggle(function ()
				{
					plus.text("-");
					info.css(
						{
							"padding-left": "20px",
							"padding-top": "5px",
							"padding-bottom": "5px"
						});
					info.show();
				}, 
				function ()
				{
					plus.text("+");
					info.hide();
				});

	var td_toggle = $('<td>').attr("valign", "top");
	td_toggle.append(plus);
	tr.append(td_toggle);

	var td_detail = $('<td>');
	td_detail.append( fmt_display_path(item_i, cslabels, revno)  );
	td_detail.append( fmt_download_path(item_i, cslabels) );
	td_detail.append( info );
	tr.append(td_detail);

	return tr;
}
	
function showChangedFiles(status, div, revno)
{
	// See sg_vv2__status__summarize.c
	// See sg_wc__status__classic_format.c
	// Since this is a historical status (and not 
	// something based on the WD) we don't need
	// to think about Resolved/Unresolved/Lost/Found/etc.

	var cslabels = ["A", "B"];
	var tbl = $('<table>');

	$.each(status, function (i, item_i) { if (item_i.status.isAdded)              tbl.append( fmt_added(     item_i, cslabels, revno) ); });
	$.each(status, function (i, item_i) { if (item_i.status.isNonDirModified)     tbl.append( fmt_modified(  item_i, cslabels, revno) ); });
	$.each(status, function (i, item_i) { if (item_i.status.isModifiedAttributes) tbl.append( fmt_attributes(item_i, cslabels, revno) ); });
	$.each(status, function (i, item_i) { if (item_i.status.isRemoved)            tbl.append( fmt_removed(   item_i, cslabels, revno) ); });
	$.each(status, function (i, item_i) { if (item_i.status.isRenamed)            tbl.append( fmt_renamed(   item_i, cslabels, revno) ); });
	$.each(status, function (i, item_i) { if (item_i.status.isMoved)              tbl.append( fmt_moved(     item_i, cslabels, revno) ); });

    if (div)
    {
        div.append(tbl);
    }
    else
    {
        $('#changes').append(tbl);
    }
}

/*****************************************************************
 *****************************************************************/

function setChangedFiles(data, parents, parentCount)
{
    var browseHref = sgCurrentRepoUrl + "/browsecs.html?recid=" + gcscsid + "&revno=" + data.description.revno;

    $('#browse, #browse2, #browse3').attr("href", browseHref);

    var select = $('#parentselection');
    var selected = false;

    $.each(parents, function (i, v)
    {
        revnoToRecid[v.revno] = i;

        var opttext = "Show changes relative to parent " + v.revno + ":" + i.slice(0,10)
                      + ", merging in " + v.mergedInCount + " new changeset"+((v.mergedInCount==1)?'':'s');
        var option = $('<option>').text(opttext).val(v.revno);
        select.append(option);
        if(v.revno==certainDiffToShow){
            select.val(v.revno);
            selected = true;
        }

        var spanParent = $('<h2 id="title_parent_revno_'+v.revno+'">').addClass("innerheading");
        if(certainDiffToShow && v.revno!=certainDiffToShow){
            spanParent.css("display", "none");
        }

        if(parentCount==1){
            spanParent.text("File Changes");
        }
        else{
            spanParent.text("Changes relative to parent ");
            var link = makePrettyRevIdLink(v.revno, i, false, null, null, sgCurrentRepoUrl);
            spanParent.append(link);
            _changesetHover.addHover(link, i, 20);
        }
        $('#changes').append(spanParent);

        var divChanges = $('<div id="diff_parent_revno_'+v.revno+'">').addClass("innercontainer");
        if(certainDiffToShow && v.revno!=certainDiffToShow){
            divChanges.css("display", "none");
        }

		if(parentCount>1){
			var mergeSummary = $('<div class="mergesummary"></div>');
			mergeSummary.append("These file changes represent the merging in of parent node");
			if(parentCount>2){mergeSummary.append("s");}
			mergeSummary.append(" ");
			var n=0;
			$.each(parents, function (iOther, vOther){
				if(iOther!=i){
					++n;
					if(n>1){
						mergeSummary.append(", ");
					}
					var link = makePrettyRevIdLink(vOther.revno, iOther, false, null, null, sgCurrentRepoUrl);
					mergeSummary.append(link);
					_changesetHover.addHover(link, iOther, 20);
				}
			})
			if(v.mergedInCount>(parentCount-1)){
				mergeSummary.append("bringing in a total of "+v.mergedInCount+" new changesets.");
			}
			divChanges.append(mergeSummary);
		}

        var ul = $('<ul>');
        var li = $('<li>');
        var div = $('<div>').attr({ id: i });
        showChangedFiles(data[i], div, v.revno);
        div.append($('<br/>'));

        li.append(div);
        ul.append(li);
        divChanges.append(ul);
        $('#changes').append(divChanges);
        populatedDiffViews.push(v.revno);
    });

    select.append($('<option>').text('Show changes relative to each parent').val('all'))
    if(!selected){
        select.val('all');
    }

    select.change(function(){
        var newRevno = $(this).val();
        if(newRevno!=certainDiffToShow && newRevno!="all"){
            select.attr('disabled', 'disabled');
            $('#parentselectionloading').css({visibility: "visible"});
            nodeRotateFunctions["tablemergereview"](0, newRevno);
        }
        changesetDetailsShowCertainDiff(newRevno);
    });
}

function setParents(parents)
{
	var nav = $('#parent_nav');
	var any = false;

	if (parents)
	{
		var ul = $('<ul></ul>').appendTo(nav);
		var img = { 'img': sgStaticLinkRoot + "/img/bluearrowL.png", 'alt': '<' };

		$.each(parents, function(i, v)
		{
		    var a = makePrettyRevIdLink(v.revno, i, false, img, null, sgCurrentRepoUrl);
		    _changesetHover.addHover(a, i, 20);

			$('<li>').append(a).appendTo(ul);

			any = true;
		});
	}

	if (! any)
		nav.hide();
}

function setChildren(children)
{
	var nav = $('#child_nav');

	if (children && children.length > 0)
	{
		var ul = $('<ul></ul>').appendTo(nav);
		var img = { 'img': sgStaticLinkRoot + "/img/bluearrowR.png", 'alt': '>' };

	    $.each(children, function (i, v)
	    {
	        var a = makePrettyRevIdLink(v.revno, v.changeset_id, false, null,
							img, sgCurrentRepoUrl);

	        _changesetHover.addHover(a, v.changeset_id, 20);

			$('<li>').append(a).appendTo(ul);
	    });
	}
	else
		nav.hide();
}

function deleteStamp(stamp)
{

    var url = sgCurrentRepoUrl + "/changesets/" + gcscsid + "/stamps/" + encodeURIComponent(stamp);
	vCore.ajax(
	{
		type : "DELETE",
		url : url,
		success : function(data, status, xhr)
		{
			var stamps = [];
			$.each(data, function(i, v)
			{
				stamps.push(v.stamp);
			});
			setStamps(stamps);

		}
	});

}

function setStamps(stamps)
{
	$('#stamps span').remove();
	if (stamps != null && stamps.length > 0)
	{

		var div = $('#stamps');

		$.each(stamps, function (i, v)
		{
		    var a = $("<a>").attr(
			{
			    "title": "view changesets with this stamp",
			    href: sgCurrentRepoUrl + "/history.html?stamp=" + encodeURIComponent(v)
			});
			a.text(v);
		    var del = $('<a>').attr(
			{
			    "title": "delete this stamp",
			    href: "#"
			});

			$('<img alt="x" />')
				.attr('src', sgStaticLinkRoot + "/img/blue_delete_short.png")
				.appendTo(del);

		    var cont = $('<span>').addClass("cont");
		    var span = $('<span>').append(a).addClass("stamp");
		    cont.append(span);
		    cont.append(del);

		    div.append(cont);
		    del.click(function (e)
		    {
		        e.preventDefault();
		        e.stopPropagation();
		        confirmDialog($("<span>").text("Delete " + v + "?"), del, deleteStamp, v);

		    });
		});
	}

}

function getAllStamps()
{
    var url = sgCurrentRepoUrl + "/changesets/" + gcscsid +"/stamps.json";

	var allstamps = [];
	vCore.ajax(
	{
		url : url,
		dataType : 'json',
		error : function(xhr, textStatus, errorThrown)
		{
			$('#maincontent').removeClass('spinning');
			reportError(null, xhr, textStatus, errorThrown);
		},
		success : function(data, status, xhr)
		{

			if ((status != "success") || (!data))
			{
				return;
			}

			$.each(data, function(i, v)
			{
				allstamps.push(v.stamp);
			});
		}

	});

}


function postComment()
{

	var url = sgCurrentRepoUrl + "/changesets/" + pageCurrentCsid + "/comments.json";

	var text = $('#inputcomment').val().trim();

	if (!text)
	{
		reportError("comment can not be empty", null, "error");
		return;
	}

	$('#maincontent').addClass('spinning');

	vCore.ajax(
	{
		type : "POST",
		url : url,
		data : JSON.stringify({text: text}),
		success : function(data, status, xhr)
		{
			setComments(data);
			$('#maincontent').removeClass('spinning');
		},
		error : function(xhr, textStatus, errorThrown)
		{
			$('#maincontent').removeClass('spinning');
		}

	});

	$('#inputcomment').val('');
}

function getStamps()
{

	$('#maincontent').css("height", "300px");

	var str = "";

	var url = sgCurrentRepoUrl + "/changesets/stamps";
	stamps = [];
	vCore.ajax(
	{
		url : url,
		dataType : 'json',

		success : function(data, status, xhr)
		{
			$('#maincontent').css("height", "");
			if ((status != "success") || (!data))
			{
				return;
			}
			$.each(data, function(i, v)
			{
				stamps.push(v.stamp);
			});
		}

	});

}

function postStamp()
{
	var url = sgCurrentRepoUrl + "/changesets/" + gcscsid + "/stamps.json";
	var text = $('#inputstamp').val().trim();

	var error = validate_object_name(text, get_object_name_constraints());
	if (error)
	{
	    err = ["Stamps " + error];
	    bubbleError.displayInputError($("#inputstamp"), err);
	    return;
	}
	vCore.ajax(
	{
		type : "POST",
		url : url,
		data: JSON.stringify({ text: text }),
		success : function(data, status, xhr)
		{
			var stamps = [];
			$.each(data, function(i, v)
			{
				stamps.push(v.stamp);
			});
			setStamps(stamps);
			$('#inputstamp').val("");
			getAllStamps();
		},
		error : function(xhr, textStatus, errorThrown)
		{
			$('#comments').removeClass('spinning');
		}

	});

	$('#inputcomment').val('');
}

function validateStamp(stamp)
{
//	var noInvalidChars = /^[^,\\\/\r\n]*$/;

	return validate_object_name(stamp, get_object_name_constraints());


}

function moveTagToThis(oldCsid)
{
	var text = $('#inputtag').val();
	var url = sgCurrentRepoUrl + "/changesets/" + oldCsid + "/tags/" + encodeURIComponent(text);
	deleteTag(text, url, postTag);

}

function deleteTag(tag, delurl, onSuccess)
{

    var url = delurl || sgCurrentRepoUrl + "/changesets/" + gcscsid +"/tags/" + encodeURIComponent(tag);
	vCore.ajax(
	{
		type : "DELETE",
		url : url,
		success : function(data, status, xhr)
		{
			if (onSuccess)
			{
				onSuccess();
			}
			else
			{
				var tags = [];
				$.each(data, function(i, v)
				{
					tags.push(v.tag);
				});
				setTags(tags);
			}

		}

	});

}

function setTags(tags)
{
	$('#tags span').remove();
	if (tags != null && tags.length > 0)
	{

	    var div = $('#tags');

		$.each(tags, function(i, v)
		{

			var span = $('<span>').text(v).addClass("vvtag");
			var del = $('<a>').attr(
			{
				"title" : "delete this tag",
				href : "#"
			});
			$('<img alt="x" />')
				.attr('src', sgStaticLinkRoot + "/img/blue_delete_short.png")
				.appendTo(del);

			var cont = $('<span>').addClass("cont");

			del.click(function(e)
			{
				e.preventDefault();
				e.stopPropagation();

				confirmDialog($("<span>").text("Delete " + v + "?"), del, deleteTag, v);
			});
			cont.append(span);
			cont.append(del);
			div.append(cont);
		});

	}
}


function postTag()
{
    var url = sgCurrentRepoUrl + "/changesets/" + gcscsid + "/tags.json";

	var text = $('#inputtag').val().trim();
	var error = validate_object_name(text, get_object_name_constraints());

	if (error)
	{
	    err = ["Tags " + error];
        bubbleError.displayInputError($("#inputtag"), err);
		return;
	}

	vCore.ajax(
		{
			type: "POST",
			url: url,
			data: JSON.stringify({ text: text }),
			success: function (data, status, xhr)
			{
				if (data.description)
				{
					var div = $('<div>').text(
							"The tag exists on this changesent, move tag?");
					div.append($('<br/><br/>'));
					var a = ($('<a>')).attr(
							{
							    href: sgCurrentRepoUrl + "/changeset.html?recid=" + data.description.changeset_id,
								"title": "view changeset",
								"target": "_blank"
							}).text(data.description.changeset_id);
					div.append(a);
					div.append($('<br/>'));
					setTimeStamps(data.description.audits, div);
					div.append($('<br/>'));
					div.append($('<br/>'));
					if (data.description.comments
							&& data.description.comments.length > 0)
					{
						data.description.comments
								.sort(function (a, b)
								{
									return (firstHistory(a).audits[0].timestamp - firstHistory(b).audits[0].timestamp);
								});
						div.append(data.description.comments[0].text);
					}
					confirmDialog(div, $('#inputtag'), moveTagToThis,
							data.description.changeset_id);
				}
				else
				{

					var tags = [];
					$.each(data, function (i, v)
					{
						tags.push(v.tag);
					});
					setTags(tags);
					$('#inputtag').val("");
				}
			}
		});

	$('#inputcomment').val('');
}

function bindObservers()
{
	if (!isTouchDevice())
	{
		$("#changes .cslist li").each(function(index)
		{
			$(this).find("span.linkSpan").first().hide();
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

function getChangeSet()
{

    var listUrl = sgCurrentRepoUrl + "/changesets/" + gcscsid + ".json?details";

    $('#csLoading .statusText').text("Loading Changeset...");
    $('#csLoading').show();

    vCore.ajax(
	{
	    url: listUrl,
	    dataType: 'json',

	    error: function (xhr, textStatus, errorThrown)
	    {
	        $('#csLoading').hide();
	    },
	    success: function (data, status, xhr)
	    {
	        var revno = data.description.revno + ":"
					+ data.description.changeset_id.slice(0, 10);
	        pageCurrentCsid = data.description.changeset_id;

	        changeset = data;
	        $('#revid').text(data.description.revno + ":" + data.description.changeset_id);
	        $('#revid_short').text(revno);
	        $('#revid_short').append(addFullHidDialog(data.description.changeset_id));
	        $('#current_nav').text(revno);
	        vCore.setPageTitle(revno);
	        $('#zip_repo').attr(
			{
			    'title': 'download zip of repo at this version',
			    href: sgCurrentRepoUrl + "/zip/" + data.description.changeset_id + ".zip"
			});

			if(data.description.audits){
				for(var i=0; i<data.description.audits.length; ++i){
					var tdwho = $('<td>');
					var tdwhen = $('<td>');
					setTimeStamps([data.description.audits[i]], null, null, tdwho, tdwhen);
					var trwho = $('<tr><th>Committed by:</th></tr>').append(tdwho);
					var trwhen = $('<tr><th>On:</th></tr>').append(tdwhen);
					trwho.insertBefore($('#trComment'));
					trwhen.insertBefore($('#trComment'));
				}
			}
	        setContainingBranches(data.description.containingBranches);
	        setComments(data.description.comments);
	        setParents(data.description.parents);
	        setChangedFiles(data, data.description.parents, data.description.parent_count);
	        setChildren(data.description.children);
	        setTags(data.description.tags);
	        setStamps(data.description.stamps);
	        bindObservers();

			vCore.trigger('dataloaded', data);
			
			var countParents = 0;
			for(x in data.description.parents){++countParents;};
			if(countParents>1){
				var moimeme = {};
				moimeme[gcscsid]=true;
				mergeReviewPane = newMergeReviewPane({
					tableId: "tablemergereview",
					head: gcscsid,
					mode: "changeset details",
					boldChangesets: moimeme,
					onResultsFetched: function(results, lastChunk){
						var baselineRevno = results[0].displayChildren[0];
						var baselineRecid = revnoToRecid[baselineRevno];
						changesetDetailsShowCertainDiff(baselineRevno);
						if(lastChunk){
							$('#mergereviewmorebutton').hide();
							$('#mergereviewmoreloading').hide();
							mergeReviewPane.hg.setHighlight(results[results.length-1].changeset_id);
						}
						else{
							$('#mergereviewmorebutton').show();
							$('#mergereviewmoreloading').show();
							$('#mergereviewmorebutton').removeAttr('disabled');
							$('#mergereviewmoreloading').css({visibility: "hidden"});
						}
						$('#parentselection').show();
						$('#mergereviewsection').show();
						mergeReviewPane.redraw();
						$('#parentselection').val(baselineRevno);
						$('#parentselection').removeAttr('disabled');
						$('#parentselectionloading').css({visibility: "hidden"});
					},
					onBeginRotateNode: function(mergeNode, newBaselineRevno){
						if(mergeNode.Index==0){
							$('#parentselectionloading').css({visibility: "visible"});
							$('#parentselection').attr('disabled', 'disabled');
						}
					}
				});
			}
	    },
	    complete: function(){
	        $('#csLoading').hide();
	        $("#csContent").show();
	        if(mergeReviewPane){
	            mergeReviewPane.redraw();
	        }
	    }
	});
}

function addBuildsToTable(table, builds)
{
	var lastname;
	$.each(builds, function(i, b)
		   {
			   if (b.name != lastname)
			   {
				   lastname = b.name;
				   var tr = $(document.createElement("tr"));
				   var td = $(document.createElement("td"));
				   td.attr("colspan", "2");

				   var buildtext = b.name;
				   if (b.branchname != null)
					   buildtext += " (" + b.branchname + ")";
				   td.text(buildtext);
				   tr.append(td);
				   table.append(tr);
			   }

			   tr = $(document.createElement("tr"));
			   td = $(document.createElement("td"));

			   if (b.icon != null)
			   {
				   var a = $(document.createElement("a"));
				   a.attr("href", "build.html?recid=" + b.recid);
				   a.attr("title", b.statusname);
				   var img = $(document.createElement("img"));
				   img.attr("src", sgStaticLinkRoot + "/img/builds/" + b.icon + "16.png");
				   a.append(img);
				   td.append(a);
			   }
			   tr.append(td);

			   td = $(document.createElement("td"));
			   a = $(document.createElement("a"));
			   a.attr("href", "build.html?recid=" + b.recid);
			   a.attr("title", b.statusname);
			   a.text(b.envname);
			   td.append(a);
			   tr.append(td);

			   table.append(tr);
		   });
}

function getBuildRequest()
{
	vCore.ajax(
	{
		url : vCore.getOption("currentRepoUrl") + "/builds/changeset-page.json",
		dataType : "json",
		data: {
			csid: gcscsid
		},

		success : function(data, status, xhr)
		{
			//vCore.consoleLog(JSON.stringify(data, null, 4));

			if (data.builds != null && data.builds.length > 0)
			{
				var table = $(document.createElement("table"));
				table.attr("id", "buildsTable");
				addBuildsToTable(table, data.builds);
				$("#builds").append(table);
				$("#buildSection").show();
			}
			else
				$("#builds").append("No builds of this changeset.");

			if (data.environments != null && data.environments.length > 0)
			{
				if (data.config != null && data.config.length > 0)
				{
					var envList = $("#buildEnvs");
					$.each(data.environments, function(i, env)
						{
							var id = 'build' + env.recid;
							var cb = $('<input type="checkbox" />').
								val(env.recid).
								attr('id', id);
							var lbl = $('<label />').
								attr('for', id).
								text(env.name);

							var tr = $("<tr>").
								appendTo(envList);
							$("<td>").
								append(cb).
								appendTo(tr);
							$("<td>").
								append(lbl).
								appendTo(tr);
						});

					$("#buildSection").show();
					$("#buildReq").show();
				}
			}
		}
	});

}

function QBuild()
{
	var buildEnvs = $("#buildEnvs input:checked");

	// Make sure at least one environment is selected
	if( buildEnvs.length == 0 )
	{
		alert("You must select at least one environment");
		return;
	}

	$("#newBuildForm select, #newBuildForm input[type=checkbox]").attr("disabled", "1");
	$("#buildBtn").hide();
	$("#newBuildSpinner").show();

	var newBuildEnvs = new Array(buildEnvs.length);
	$.each(buildEnvs, function(i, opt)
		   {
			   newBuildEnvs[i] = $(opt).val();
		   });

	var data = new Object();
	data.csid = gcscsid;
	data.priority = parseInt($($("#buildPriority option:selected")[0]).attr("value"));
	data.envs = newBuildEnvs;

	vCore.ajax(
	{
		type: "POST",
		dataType : "json",
		contentType : "application/json",
		url : vCore.getOption("currentRepoUrl") + "/builds.json",
		data : JSON.stringify(data),

		success : function(data, status, xhr)
		{
			//vCore.consoleLog(JSON.stringify(data, null, 4));

			var newTable = false;
			if ($("#buildsTable").length == 0)
			{
				newTable = true;
				var table = $(document.createElement("table"));
				table.attr("id", "buildsTable");
			}
			else
				table = $("#buildsTable");

			addBuildsToTable(table, data);

			if (newTable)
			{
				$("#builds").empty();
				$("#builds").append(table);
				$("#builds").show();
			}
		},

		complete : function(xhr, status)
		{
			$("#newBuildSpinner").hide();
			$("#buildBtn").show();
			$("#newBuildForm select, #newBuildForm input[type=checkbox]").removeAttr("disabled");
		}
	});
}

function loadChangeSet(csid)
{
	gcscsid = csid;

    fv = new fileViewer();
    _changesetHover = new hoverControl(fetchChangesetHover);
    _wiHover = new hoverControl(fetchWorkItemHover);
    bubbleError = new bubbleErrorFormatter();
    stampError = bubbleError.createContainer($('#inputstamp'));
    $('#inputstamp').after(stampError);
    tagError = bubbleError.createContainer($('#inputtag'));
    $('#inputtag').after(tagError);
    $('#inputstamp').keyup(function (e1)
    {
        if (e1.which == 13)
        {
            e1.preventDefault();
            e1.stopPropagation();
            postStamp();
        }
    });

    $('#inputtag').keyup(function (e1)
    {
        if (e1.which == 13)
        {
            e1.preventDefault();
            e1.stopPropagation();
            postTag();
        }
    });

    $('#mergereviewmorebutton').click(function(e){
        mergeReviewPane.fetchMoreResults();
        $('#mergereviewmorebutton').attr('disabled', 'disabled');
        $('#mergereviewmoreloading').css({visibility: "visible"});
    });

	_enableOnInput('#submit', '#inputcomment');
	_enableOnInput('#addtag', '#inputtag');
	_enableOnInput('#addstamp', '#inputstamp');

	getChangeSet();
	getAllStamps();
	getBuildRequest();
	
	if($.browser.msie){
		var fixDivHeight = function(){
			$('#div_that_ie_screws_up').height("25em");
		};
		$('#mergereviewmorebutton').hover(fixDivHeight, fixDivHeight);
	}
}
