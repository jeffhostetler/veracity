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

var _blockedItems = false;
var _blockingItems = false;
var _linkItems = [];
var _duplist = false;
var _csassoc = false;
var _cslinks = [];
var _editModeButtons = [];
var _viewModeButtons = [];
var _blocklist = [];
var _blockerlist = [];
var _csvals = [];
var _userTexts = [];

var wiSections = {};
var _wirecid = false;
var _wit = false;
var wiVform = false;
var workController = false;
var editMode = false;
var fromQuery = false;
var queryResultsObj = {};
var _showHistory = false;
var wiProgress = null;
var wiCommentController = null;
var stampsController = null;
var redirecturl = "";
var lock = false;
var _changesetHover = null;
var _vvrightsidebar = null;
var _vvmarkdown = null;
var navHandled = false;

function _truncComment(txt)
{
	var prefix = "";
	var cmt = txt || '';

	var matches = txt.match(/^([0-9]+:[a-f0-9]+…\t)(.*)/);

	if (matches)
	{
		prefix = matches[1];
		cmt = matches[2];
	}

	cmt = cmt.substring(0,140);

	matches = cmt.match(/^([^\r\n]+)[\r\n]/);

	if (matches)
		cmt = matches[1];

	return(prefix + cmt);
}

function appendRelation(tbl, rel, enabled, isCs, value)
{
	var link = sgCurrentRepoUrl + "/workitem.html?recid=" + rel.other;
	var csid = null;
	var repurl = sgCurrentRepoUrl;
	var checkField = 'other';

	value = value || '';

	if (isCs)
	{
		var commit = rel.commit;
		csid = commit;
		checkField = 'commit';

		var matches = commit.match(/^(.+):(.+)/);

		if (matches)
		{
			repurl = repurl.replace("repos/" + encodeURIComponent(startRepo), "repos/" + encodeURIComponent(matches[1]));
			csid = matches[2];
		}

		link = repurl + "/changeset.html?recid=" + csid;
	}

	var rows = $(tbl).find('tr.deleted, tr.moved');
	rows.each(
		function(i, trraw)
		{
			var tr = $(trraw);
			var d = tr.data('rel');
			var sameField = d[checkField] == rel[checkField];

			if (sameField && (checkField == 'other'))
				sameField = (d.relationship == rel.relationship);

			if (sameField)
				tr.remove();
		}
	);

	var tr = $('<tr>').
		data('rel', rel);
	var deleter = $('<td class="deleter" />').
		appendTo(tr);

	var td = $(document.createElement('td')).
		appendTo(tr);

	if (value && (value != 'commits'))
	{
		var sel = $('<select />').
			css('display', 'inline').
			addClass('reltype').
			appendTo(td);

		var vals = {
			"blockers": "Depends On",
			"blocking": "Blocking",
			"duplicateof": "Duplicate Of",
			"duplicates": "Duplicated By",
			"related": "Related To"
		};

		for ( var v in vals )
		{
			var o = $('<option />').
				text(vals[v]).
				val(v).
				appendTo(sel);
		}

		sel.val(value).data('previous', value);

		sel.change(
			function()
			{
				var newRow = tr.clone(false).addClass('moved').appendTo(tbl);

				var newData = {};
				var d = tr.data('rel');
				for ( var fn in d )
					newData[fn] = d[fn];
				newData._delete = true;
				newRow.data('rel', newData);

				var ss = newRow.find('select');
				ss.val(sel.data('previous'));
				sel.data('previous', sel.val());
			}
		);
	}

	var a = $('<a />').attr('href', link).text(rel.id || '').
		appendTo(td);
	var id = a;

	if (isCs)
		_changesetHover.addHover(a, csid, null, null, repurl);
	else
	    _menuWorkItemHover.addHover(a, rel.other);

	a.click(function (e)
	{
	    addRemoteLinkClickEvent(e, $(this));
	});
	var isClosed = false;

	if (! isCs)
		isClosed = rel.status && (rel.status != 'open');

	var ttext = " ";
	if (rel.title)
		ttext += rel.title;

    var vc = new vvComment(ttext);
	var title = $('<span />').html(vc.summary);
	if(rel.branches && rel.branches.length>0){
		for(var i=rel.branches.length; i>0; --i){
			title.prepend($('<a></a>').text(rel.branches[i-1]).attr("href", branchUrl(rel.branches[i-1], rel.repoName)));
			if(rel.branches.length>1){
				if(i==rel.branches.length){title.prepend(" and ");}
				else if (i!=1){title.prepend(", ");}
			}
		}
		title.prepend(" in ");
	}
	if (isCs)
		title.addClass('cstitle');
	td.append(title);

	if (isClosed)
	{
		id.addClass('relclosed');
		title.addClass('relclosed');
	}

	a = $(document.createElement('a')).
		attr('title', 'delete this relation').
		attr('href', '#').
		appendTo(deleter);

	var x = $(document.createElement('img')).
		attr('src', sgStaticLinkRoot + "/img/red_delete.png").
		attr('alt', '[-]').
		css('vertical-align', 'text-bottom').
		appendTo(a);

	if (enabled)
		deleter.show();
	else
		deleter.hide();

	a.click(
		function(e) {
			e.stopPropagation();
			e.preventDefault();

			tr.addClass('deleted');

			var a = $(tr.find('a')[0]);
			var ctr = a.closest('td');
			var txt = a.text();
			ctr.text(txt);
		}
	);

	tr.appendTo(tbl);

	tbl.closest('.relctr').removeClass('emptydrop');

	// incomplete data from, e.g., visited-items list - flesh it out
	if (rel.fillIn)
	{
		if (isCs)
		{
			repurl = sgCurrentRepoUrl;
			var commit = rel.commit;

			var matches = commit.match(/(.+):(.+)/);

			if (matches)
			{
				var repo = matches[1];
				commit = matches[2];

				if (repo != startRepo)
					repurl = repurl.replace("repos/" + encodeURIComponent(startRepo), "repos/" + encodeURIComponent(repo));
			}

			var u = repurl + "/changesets/" + commit + ".json";

			vCore.ajax(
				{
					url: u,
					dataType: 'json',
					noFromHeader: true,
					success: function(data) {
						var desc = data.description;

						id.text(desc.revno + ":" + desc.changeset_id.substring(0,10) + "…");

						if (desc.comments && desc.comments.length > 0)
							title.text(desc.comments[0].text.substring(0, 140));
					}
				}
			);
		}
		else
		{
			var u = sgCurrentRepoUrl + "/workitem/" + rel.other + ".json?_fields=status,title,id";

			vCore.ajax(
				{
					url: u,
					dataType: 'json',
					success: function(data) {
						id.text(data.id);
						title.text(" " + data.title);

						if (data.status && (data.status != 'open'))
						{
							id.addClass('relclosed');
							title.addClass('relclosed');
						}
					}
				}
			);
		}
	}
}

function setRelations(section, contents, enabled, value)
{
	var tbl = section.find('table');
	var iscs = (value == 'commits');

	if (iscs)
		tbl.empty();
	else
	{
		var drops = tbl.find('select');

		$.each(drops,
			function(i,v)
			{
				var drop = $(v);
				if (drop.val() == value)
					drop.closest('tr').remove();
			}
		);
	}

	for ( var i = 0; i < contents.length; ++i )
	{
		var rel = contents[i];
		appendRelation(tbl, rel, enabled, iscs, value);
	}
	
	if (contents.length > 0)
		section.removeClass('emptydrop');
}

function addRelation(value, label, section, isCs)
{
	var parser = /^([A-Z0-9]+) ([a-zA-Z]+) (.+)/;

	var matches = label.match(parser);

	var newbie = {
		other: isCs ? null : value,
		commit: isCs ? value : null,
		id: label,
		status: '',
		title: '',
		fillIn: ! isCs
	};

	if (matches)
	{
		newbie = {
			other: value,
			id: matches[1],
			status: matches[2],
			title: matches[3]
		};
	}
	else
	{
		matches = label.match(/^(.+)\t(.+)/);

		if (matches)
		{
			newbie.id = matches[1];
			newbie.title = matches[2];
		}
	}

	var ctr = isCs ? wiSections.csrelations : wiSections.relations;

	var tbl = ctr.find('table');
	appendRelation(tbl, newbie, true, !! isCs, section);

	ctr.removeClass('emptydrop');
}

function addRelations(field, label, wit)
{
	var tbl = wiSections.relations.find('table');

	if ((! tbl) || (tbl.length == 0))
		tbl = $('<table>').appendTo(wiSections.relations.find('.innercontainer'));

	var contents = [];

	if (wit && wit[field] && wit[field].length)
		contents = wit[field];

	setRelations(wiSections.relations, contents, false, field);

	var f = {
		_data: {},
		_onChange : null,

		val: function(vals)
		{
			if (vals)
				setRelations(wiSections.relations, vals, false, field);
			else
			{
				vals = [];
				var seenLive = {};

				wiSections.relations.find('tr').each(
					function(i, trraw)
					{
						var tr = $(trraw);

						var sel = tr.find('select');
						if (sel.val() == field)
						{
							var rec = tr.data('rel');

							if (tr.is('.deleted') || tr.is('.moved'))
								rec._delete = true;
							else
								seenLive[rec.other] = true;

							vals.push( rec );
						}
					}
				);

				for ( var i = vals.length - 1; i >= 0; --i )
					if ((vals[i]._delete) && seenLive[vals[i].other])
						vals.splice(i, 1);
			}

			return(vals);
		},

		change: function(f) {
			this._onChange = f;
		},

		/*
		 * no proper sorting, just a quick equal/unequal
		 */
		_compareValues: function(v, lastval) {
			v = v || [];
			lastval = lastval || [];
			var result = v.length - lastval.length;

			if (result != 0)
				return(false);

			var sortrels = function(a,b) {
				if (a.other < b.other)
					return(-1);
				else if (a.other > b.other)
					return(1);
				else
					return(0);
			};

			lastval.sort(sortrels);
			v.sort(sortrels);

			for ( var i = 0; i < v.length; ++i )
			{
				if (v[i].other != lastval[i].other)
					return(false);
				if (v[i]._delete)
					return(false);
			}

			return(true);
		},

		onChange: function()
		{
			if (this._onChange)
				this._onChange(wiVform);
		},

		data: function(vari, value) {
			if (value)
				this._data[vari] = value;

			return( this._data[vari] );
		},

		attr: function(vari, value) {
			return( wiSections.relations.attr(vari, value) );
		},

		is: function(condition) {
			return( wiSections.relations.is(condition) );
		}
	};

	var vf = new vFormField(wiVform, null, f, null, null, { name: field, objectAllowed: true });
	vf.data('lastval', contents);
	wiVform.addCustomField(field, vf, { name: field });

	wiSections.relations.droppable(
		{
			// todo: confirm it's an li from (via .accept)
			drop: function(e, ui) {
				var dragged = ui.draggable;

				if (dragged[0].nodeName.toLowerCase() == 'li')
				{
					var rel = dragged.data('rel');
					rel.fillIn = true;
					rel._delete = false;
					appendRelation(tbl, rel, true, false, 'related');
					dragged.css('position', 'relative');
					return(false);
				}
			},

			hoverClass: 'relhover',
			activeClass: 'relactive',

			accept: function(tr){
				var rel = tr.data('rel');
				if (rel && rel.other)
				{
					var ours = f.val() || [];
					var ourRecid = null;

					if (wiVform && wiVform.record)
						ourRecid = wiVform.record.recid || null;

					if (ourRecid == rel.other)
						return(false);

					for ( var i = 0; i < ours.length; ++i )
						if ((ours[i].other == rel.other) && ! ours[i]._delete)
							return(false);

					return(true);
				}

				return(false);
			}
		}
	);


	wiSections.relations.data(field, f);

	return(wiSections.relations);
}


function addCsRelations(field, label, wit)
{
	var section = $('<div class="relations" />').
		attr('id', field).
		append($('<h3 class="innerheading">' + label + '</h3>')).
		appendTo(wiSections.csrelations);

	var ctr = $('<div class="innercontainer"></div>').
		appendTo(section).
		append('<p class="dropplaceholder">Drag Commits from the Activity Stream, or select from the Search box below.</p>');

	var tbl = $('<table></table>').
		appendTo(ctr);

	var contents = [];

	if ((! wit) || (! wit[field]) || (! wit[field].length))
	{
		section.hide();
	}
	else
		contents = wit[field];

	setRelations(section, contents, false, 'commits');

	var f = {
		_data: {},
		_onChange : null,

		val: function(vals)
		{
			if (vals)
				setRelations(section, vals, false, 'commits');
			else
			{
				vals = [];
				section.find('tr').each(
					function(i, trraw)
					{
						var tr = $(trraw);

						var rec = tr.data('rel');
						if (tr.is('.deleted'))
							rec._delete = true;

						vals.push( rec );
					}
				);
			}

			return(vals);
		},

		change: function(f) {
			this._onChange = f;
		},

		/*
		 * no proper sorting, just a quick equal/unequal
		 */
		_compareValues: function(v, lastval) {
			v = v || [];
			lastval = lastval || [];
			var result = v.length - lastval.length;

			if (result != 0)
				return(false);

			var sortrels = function(a,b) {
				if (a.other < b.other)
					return(-1);
				else if (a.other > b.other)
					return(1);
				else
					return(0);
			};

			lastval.sort(sortrels);
			v.sort(sortrels);

			for ( var i = 0; i < v.length; ++i )
			{
				if (v[i].other != lastval[i].other)
					return(false);
				if (v[i]._delete)
					return(false);
			}

			return(true);
		},

		onChange: function()
		{
			if (this._onChange)
				this._onChange(wiVform);
		},

		data: function(vari, value) {
			if (value)
				this._data[vari] = value;

			return( this._data[vari] );
		},

		attr: function(vari, value) {
			return( section.attr(vari, value) );
		},

		is: function(condition) {
			return( section.is(condition) );
		}
	};

	section.data('field', f);
	var vf = new vFormField(wiVform, null, f, null, null, { name: field, objectAllowed: true });
	vf.data('lastval', contents);
	wiVform.addCustomField(field, vf, { name: field });

	wiSections.csrelations.droppable(
		{
			drop: function(e, ui) {
				var dragged = ui.draggable;

				var rel = dragged.data('rel');
				rel._delete = false;
				rel.fillIn = true;

				if (rel.repo && (rel.repo != startRepo) && ! rel.commit.match(/:/))
					rel.commit = rel.repo + ":" + rel.commit;

				appendRelation(tbl, rel, true, true, 'commits');
				dragged.css('position', 'relative');
				return(false);
			},

			hoverClass: 'relhover',
			activeClass: 'relactive',

			accept: function(tr){
				var rel = tr.data('rel');
				if (rel && rel.commit)
				{
					var ours = f.val() || [];

					for ( var i = 0; i < ours.length; ++i )
						if ((ours[i].commit == rel.commit) && ! ours[i]._delete)
							return(false);

					return(true);
				}

				return(false);
			}
		}
	);

	return(section);
}


function _reloadWorkItem(vform)
{
	var wit = vform.record;
	var witId = wit.recid;
	if (getQueryStringVal("edit") && getQueryStringVal("query"))
	{
	    redirLastQuerySession();
	    return;
	}
	var rurl = redirecturl || sgCurrentRepoUrl + "/workitem.html?recid=" + witId;

	if (getQueryStringVal("query") && !redirecturl)
	{
		rurl = $.param.querystring(rurl, { "query": true });
	}
	document.location.href = rurl;
}


function _disableFields()
{
	$('#contentWrapper input').
		attr('readonly', true).
		addClass('disabled');
	$('#contentWrapper select').
		attr('disabled', true).
		addClass('disabled');
	$('#contentWrapper textarea').
		attr('readonly', true).
		addClass('disabled');
}

function _enableFields()
{
	$('#contentWrapper input').
		removeAttr('readonly').
		removeClass('disabled');
	$('#contentWrapper select').
		removeAttr('disabled').
		removeClass('disabled');

	$('#contentWrapper textarea').
		removeAttr('readonly').
		removeClass('disabled');
}


function _hideRelSelects()
{
	var sels = $('#relations').find('select');

	$.each(
		sels,
		function(i,sel)
		{
			var s = $(sel);
			var span = s.data('span');

			if (! s.data('span'))
			{
				span = $("<span class='relspan'></span>");
				s.data('span', span);

				s.after(span);
			}

			span.text(s.find("option:selected").text() + " ");
			s.hide();
			span.show();
		}
	);
}

function _showRelSelects()
{
	$('.relspan').hide();
	$('#relations select').show();
}


function _enterEditMode()
{
	editMode = true;
	$('.markdowneditor').show();
	wiVform.markdowneditor.editor.refreshPreview();
	$('.labeldescreader').hide();
	_showRelSelects();

	$('.ui-draggable').draggable('enable');
	$.each(_editModeButtons,
		function(i,b) {
			showEl(b, 'inline-block');
		}
	);

	$('#maincontent, #rightSidebar').addClass('editable');

	$('#editwi, #editwitop, #addcomment, #backtolist').hide();
	$('#topbuttons, #reladder, #csreladder').show();
	vvWiHistory.disable();

	$('.relations').show();
	$('.relations .deleter').show();

	$('#wiheader').removeClass('wiheader');
	$('.validationerror').hide();
	wiSections.attachments.hide();
	
	wiVform.enable();

	if (! _wit.recid)
	{
		$('#statblock').hide();

		$("#item_milestone").closest('div').addClass('firstwimeta');

		wiVform.getField("created_date").hide();

		$('#canceledit, #canceledittop').hide();
		wiSections.attachments.hide();

		wiVform.setRecord( {
			reporter : sgUserName,
			status: 'open',
			rectype: 'item',
			milestone: sgCurrentSprint || ""
		} );
	}
	else
	{
		$('#statblock').show();
	 	workController.enable();
	    workController.form.options.newRecord = false;
	    wiVform.addSubForm(workController.form);	 
	}

	wiCommentController.disable();
	stampsController.enable();

	$('.relctr').css('width', '48%');

	vCore.resizeMainContent();
	
	utils_doneSaving();
}



function _exitEditMode()
{
	$.each(_editModeButtons,
		   function(i,b) {
			   hideEl(b);
		   }
		  );

	_hideRelSelects();
	$('.markdowneditor').hide();
	$('.labeldescreader').show();
	$('.ui-draggable').draggable('disable');
	$('.ui-draggable').removeClass('ui-state-disabled');

	$('#maincontent, #rightSidebar').removeClass('editable');
	$('.relations .deleter').hide();

	$('#canceledit, #canceledittop, #topbuttons, #reladder, #csreladder').hide();
	$('#editwi, #editwitop, #addcomment, #backtolist, #topmenunewwi').show();

	vvWiHistory.enable();

	$('.validationerror').hide();
	wiSections.attachments.show();
	$('#statblock').show();

	$('#wiheader').addClass('wiheader');

	$('.editlabel').hide();

	wiVform.disable();
	if (_wit.recid)
	{
		wiVform.removeSubForm(workController.form);
		workController.disable();
	}
	stampsController.disable();
	wiCommentController.enable();

	vCore.resizeMainContent();
	editMode = false;

	var liveCount = 0;

	$('.relctr').each(
		function (i, div)
		{
			var rows = $(div).find('tr');
			if (rows.length < 1)
				$(div).hide();
			else
			{
				++liveCount;
				$(div).show();
			}
		}
	);

	if (liveCount == 1)
		$('.relctr').css('width', '100%');
	else
		$('.relctr').css('width', '48%');
}

function _addAttachment(recid, att)
{
    var dlUrl = sgCurrentRepoUrl + '/workitem/' + recid + "/attachment/" + att.recid + ".json";
    var showInBrowser = ["image/png", "image/gif", "image/jpeg"];
    var a = $(document.createElement('a'));
	var alwaysDlUrl = dlUrl;
    
    if ($.inArray(att.contenttype.toLowerCase(), showInBrowser) >= 0)
    {
        dlUrl = dlUrl += "?show=true";
        a.attr("target", "_blank");
    }
    a.text(att.filename).
		attr('title', att.filename).
		attr('href', dlUrl).
		attr('type', att.contenttype);
	var container = $("<div class='wiattachment'></div>");

	var div = $('<div />').
		addClass('wiattachmentname').
		appendTo(container).
		append(a);

	div.append(' ');

	var dl = $('<a title="download" class="deleter"></a>').
		attr('href', alwaysDlUrl).
		attr('type', att.contenttype).
		prependTo(container);

	dl.append($('<img alt="download" />').attr('src', sgStaticLinkRoot + '/img/blue_download.png'));

	var deleter = $(document.createElement('a')).
		attr('title', 'delete this attachment').
		attr('href', '#').
		addClass('deleter').
		prependTo(container);

	var x = $(document.createElement('img')).
		attr('src', sgStaticLinkRoot + "/img/red_delete.png").
		attr('alt', '[-]').
		css('vertical-align', 'text-bottom').
		appendTo(deleter);

	var outer = wiSections.attachments;

	var dodelete =
		function()
		{
			_spinUp(outer);

			var delurl = sgCurrentRepoUrl + "/workitem/" + _wit.recid + "/attachment/" + att.recid + ".json";

			vCore.ajax(
				{
					url: delurl,
					type: "DELETE",

					complete: function()
					{
						_spinDown(outer);
					},

					success: function(data)
					{
						var t = container.text();
						container.empty();
						container.text(t);
						container.addClass('deleted');
					}
				}
			);
		};

	deleter.click(
		function(e)
		{
			e.stopPropagation();
			e.preventDefault();

			confirmDialog("Delete attachment " + att.filename + "?", deleter, dodelete);
		}
	);

	return(container);
}

function _ignoreEnter(e, searcher)
{
	var key = (e.which || e.keyCode);

	if (key == 13)
	{
		if (searcher && (searcher.val().match(/^[0-9]+$/)))
			searcher.autocomplete('search', { 'text': searcher.val(), '_firenow': true });

		e.preventDefault();
		e.stopPropagation();
		return(false);
	}

	return(true);
}

function _postPopulate(rec)
{
	var itemUrl = sgCurrentRepoUrl + "/workitems.json?_fields=id,title,assignee,status,recid";
	var recid = false;

	var id;

	if (rec && rec.id)
		id = "ID: " + rec.id;
	else
		id = "New Work Item";

	$('#wititlebar').text(id);

	if (rec && rec.blockeditems)
	{
		$.each(rec.blockeditems,
			function(i, bi) {
				_blocklist[bi] = true;
			}
		);
	}
	if (rec && rec.dependents)
	{
		$.each(rec.dependents,
			function(i, dep) {
				_blockerlist[dep] = true;
			}
		);
	}

	if (!! rec && !! rec.recid)
	{
		recid = rec.recid;
	}
	else
	{
		$("#zing_item_related").hide();
	}

	var stat = $('#zing_item_status');
	stat.change();

	workController.updateTimeLogged();

	rec.comments = rec.comments || [];
	rec.attachments = rec.attachments || [];

	rec.comments.sort(
		function(a, b) {
			return( a.when - b.when );
		}
	);

	var scrollComment = null;

	$.each(rec.comments,
		function(i, cmt) {
			var added = wiCommentController.renderComment(cmt);
			scrollComment = scrollComment || added;
		}
	);

	if (scrollComment)
	{
		var tryToScroll = function()
		{
			if (vCore.ajaxCallCount > 0)
				setTimeout(tryToScroll, 100);
			else
				if (scrollComment[0].scrollIntoView)
					scrollComment[0].scrollIntoView(true);
		};

		setTimeout(tryToScroll, 100);
	}

	addRelations('blockers', "Depends On", rec);
	addRelations('blocking', "Blocking", rec);
	addRelations('duplicateof', "Duplicate Of", rec);
	addRelations('duplicates', "Duplicated By", rec);
	var relatedTo = addRelations('related', "Related To", rec);

	$.each(rec.attachments,
		   function (i, att) {
			   wiSections.attachments.append( _addAttachment(rec.recid, att));
		   }
		  );

	var relForm = $('<form id="reladder" />').
		appendTo(wiSections.relations.find('.innercontainer')).
		hide();

	var relAdder = $('<div class="relations" />').
		append($('<input type="text" class="searchinput" id="relsearch" />')).
		appendTo(relForm);

	var relsearch = $('#relsearch');

	relsearch.after(
		$("<a href='#' title='clear search' />").
		click(
			function(e) {
				relsearch.val('');
				return(false);
			}
		).
		append(
			$('<img />').
				attr('src', sgStaticLinkRoot + "/img/cancel18.png").
				attr('alt', "[X]")
		)
	);

	relsearch.keypress( _ignoreEnter );

	relsearch.autocomplete(
		{
			appendTo: relAdder,

			source: function (req, callback) {
				relsearch.data('fired', true);

				relsearch.autocomplete('close');

				var term = req.term;
				var results = [];
				var already = {};
				var alreadyList = relatedTo.data('related').val() || [];

				var recid = wiVform.getRecordValue('recid') || null;

				if (recid)
					already[recid] = true;

				for ( var i = 0; i < alreadyList.length; ++i )
					if (! alreadyList[i]._delete )
						already[ alreadyList[i].other ] = true;

				var u = sgCurrentRepoUrl + "/workitem-fts.json?term=" + encodeURIComponent(term || '');

				if (! term)
				{
					var vlist = _visitedItems() || [];

					var ids = [];
					$.each
					(
						vlist,
						function (i, v)
						{
							var nv = v.split(":");
							ids.push(nv[0]);
						}
					);

					u = sgCurrentRepoUrl + "/workitem-details.json?_ids=" + encodeURIComponent(ids.join(','));
				}

				vCore.ajax(
					{
						url: u,
						dataType: 'json',
						errorMessage: 'Unable to retrieve work items at this time.',

						success: function(data)
						{
							results = [];

							for ( var i = 0; i < data.length; ++i )
								if (! already[ data[i].value ])
								{
									results.push(data[i]);
									already[ data[i].value ] = true;
								}
						},

						complete: function()
						{
							callback(results);
						}
					}
				);
			},
			delay: 400,
			minLength: 0,
			select: function(event, ui) {
				event.stopPropagation();
				event.preventDefault();
				addRelation(ui.item.value, ui.item.label, 'related');

				relsearch.autocomplete('close');
				relsearch.focus();
				window.setTimeout(loadZero, 250);

				return(false);
			}
		}
	);

	var el = relsearch.data('autocomplete').element;
	el.oldVal = el.val;

	el.val = function(v) {
		return( el.oldVal() );
	};


	var loadZero = function() {
		if (relsearch.data('fired') || ! relsearch.data('waiting'))
			return;

		if (relsearch.val())
			return;

		relsearch.autocomplete('search', '');
	};

	relsearch.blur(
		function() {
			relsearch.data('waiting', false);
		}
	);

	relsearch.focus(
		function(e) {
			relsearch.data('fired', false);
			var v = relsearch.val();

			if (! v)
			{
				relsearch.data('waiting', true);
				window.setTimeout(loadZero, 3000);
			}
			else
			{
				window.setTimeout(
					function()
					{
						relsearch.autocomplete('search', v);
					},
					250
				);
			}
		}
	);

	var csRelated = addCsRelations('commits', "Associated Commits", rec);

	var csrelForm = $('<form id="csreladder" />').
		appendTo(wiSections.csrelations.find('.innercontainer')).
		hide();

	var repoList = $('<select id="assocrepo" title="Filter commits by repository" />');
	var opt = $('<option />').attr('value', startRepo).
		text(startRepo).
		appendTo(repoList);
	repoList.val(startRepo).hide();

	vCore.fetchRepos(
		{
			"success": function(replist) {
				$.each(replist,
					   function(i, v) {
						   var desc = v;

						   if (desc != startRepo)
						   {
							   var o = $('<option />').attr('value', desc).
								   text(desc).
								   appendTo(repoList);
							   repoList.show();
						   }
					   }
					  );
			}
		}
	);

	var userList = $('<select id="assocuser" title="Filter commits by user" />');
	opt = $('<option />').attr('value', '').
		text('-- All Users --').
		appendTo(userList);

	var updateUserList = function (users, inactiveUsers, val)
	{
		var oldval = val || userList.val() || vCore.getOption("userName") || '';

		userList.empty();
		opt = $('<option />').attr('value', '').
			text('-- All Users --').
			appendTo(userList);

		users.sort(
			function(a,b) {
				return( a.name.localeCompare(b.name) );
			}
		);

		$.each(
			users,
			function (i, v)
			{
				if (v.recid != NOBODY)
				{
					var o = $('<option />').
					attr('value', v.recid).
					text(v.name).
					appendTo(userList);
				}
			}
		);

		if (oldval)
			userList.val(oldval);
	};

	vCore.fetchUsers(
		{
			"success": updateUserList
		}
	);

	repoList.change(
		function ()
		{
			vCore.fetchUsers(
				{
					"success": updateUserList,
					"repo" : repoList.val(),
					"val": userList.val()
				}
			);
		}
	);


	var csrelAdder = $('<div class="relations" />').
		append(repoList).
		append(userList).
		append($('<input type="text" class="searchinput" id="csrelsearch" />')).
		appendTo(csrelForm);

	var csrelsearch = $('#csrelsearch');

	csrelsearch.after(
		$("<a href='#' title='clear search' />").
		click(
			function(e) {
				csrelsearch.val('');
				return(false);
			}
		).
		append(
			$('<img />').
				attr('src', sgStaticLinkRoot + "/img/cancel18.png").
				attr('alt', "[X]")
		)
	);

	csrelsearch.keypress(
		function(e) {
			_ignoreEnter(e, csrelsearch);
		}
	);

	var addSelected = function(item, refreshAfter)
	{
		var val = item.value;
		var label = item.label;

		var reponame = $('#assocrepo').val();
		if (reponame && (reponame != startRepo))
		{
			val = reponame + ":" + val;

			label = label.replace("\t", "\t (" + reponame + ") ");
		}

		addRelation(val, label, 'commits', true);

		if (refreshAfter !== false)
			csrelsearch.autocomplete('search', csrelsearch.val());
	};

	csrelsearch.autocomplete(
		{
			appendTo: csrelAdder,

			source: function (req, callback) {
				csrelsearch.data('fired', true);
				csrelsearch.autocomplete('close');

				var term = req.term || '';
				var fireatwill = false;

				if (term._firenow)
				{
					fireatwill = true;
					term = term.text;
				}
				else
					term = term.trim();

				var isNumber = term.match(/^[0-9]+$/);

				if (term && (term.length < 3) && (! isNumber))
				{
					callback([]);
					return;
				}

				var results = [];
				var already = {};
				var alreadyList = csRelated.data('field').val() || [];

				var recid = wiVform.getRecordValue('recid') || null;

				for ( var i = 0; i < alreadyList.length; ++i )
				{
					var csid = alreadyList[i].commit;
					var matches = csid.match(/^.+:(.+)/);
					if (matches)
						csid = matches[1];

					if (! alreadyList[i]._delete)
						already[ csid ] = true;
				}

				var reponame = $('#assocrepo').val();
				var repurl = sgCurrentRepoUrl;

				if (reponame)
					repurl = sgCurrentRepoUrl.replace("repos/" + encodeURIComponent(startRepo), "repos/" + encodeURIComponent(reponame));

				var u;
				var last10 = false;
				var user = userList.val();

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

						success: function(data)
						{
							results = [];

							if (last10)
							{
								var items = data.items || [];

								for ( var i = 0; i < items.length; ++i )
								{
									var item = items[i];
									var rec = {};
									var text = '';

									if (item.comments && (item.comments.length > 0))
										text = item.comments[0].text;
									text = _truncComment(text);
									rec.label = item.revno + ":" + item.changeset_id.substring(0,10) + "…" +
										"\t" + text;
									rec.text = text;
									rec.value = item.changeset_id;
									rec.csid = item.changeset_id;

									if (! already[rec.value])
									{
										results.push(rec);
										already[rec.value] = true;
									}
								}
							}
							else
							{
								if (fireatwill && isNumber && (data.length == 1) && ! already[ data[0].value ])
								{
									addSelected(data[0], false);
									results = [];
									csrelsearch.val('');
								}
								else
									for ( var i = 0; i < data.length; ++i )
										if (! already[ data[i].value ])
										{
											data[i].label = _truncComment(data[i].label);
											results.push(data[i]);
											already[ data[i].value ] = true;
										}
							}
						},

						complete: function()
						{
							callback(results);
						}
					}
				);
			},
			delay: 400,
			minLength: 0,
			select: function(event, ui) {
				event.stopPropagation();
				event.preventDefault();

				addSelected(ui.item);

				csrelsearch.autocomplete('close');
				csrelsearch.focus();
				window.setTimeout(csLoadZero, 250);

				return(false);
			}
		}
	);

	var csel = csrelsearch.data('autocomplete').element;
	csel.oldVal = csel.val;

	csel.val = function(v) {
		return( csel.oldVal() );
	};

	csrelsearch.blur(
		function() {
			csrelsearch.data('waiting', false);
		}
	);

	var csLoadZero = function() {
		if (csrelsearch.data('fired') || ! csrelsearch.data('waiting'))
			return;

		if (csrelsearch.val())
			return;

		csrelsearch.autocomplete('search', '');
	};

	csrelsearch.focus(
		function(e) {
			csrelsearch.data('fired', false);
			var v = csrelsearch.val();

			if (v)
			{
				csrelsearch.data('waiting', false);

				window.setTimeout(
					function()
					{
						csrelsearch.autocomplete('search', v);
					},
					250
				);
			}
			else
			{
				csrelsearch.data('waiting', true);
				window.setTimeout(csLoadZero, 3000);
			}
		}
	);

	vCore.fetchUsers
	(
		{
			success: function(data, inactiveUsers)
			{
				workController.modalTimePopup.setUsers(data);
				if (data && data.length)
				{
					data.sort(
						function(a, b)
						{
							return strcmp(a.name, b.name);
						}
					);
	 				
					$.each(data,
						function(i, user)
						{
							_wiusers[user.recid] = user.name;
						}
					);
				}
				if (inactiveUsers && inactiveUsers.length)
				{
					inactiveUsers.sort(
						function(a,b) {
							return( strcmp(a.name, b.name ) );
						}
					);
					$.each(inactiveUsers,
						function(i, user)
						{
							_wiusers[user.recid] = user.name;
						}
					);
				}
				_fillInUsers();
			}
		}
	);

	var editButton = $(document.createElement('input')).
		attr('type', 'button').
		attr('id', 'editwi').
		val('Edit').
		click(_enterEditMode);

	var editButtonTop = $(document.createElement('input')).
		attr('type', 'button').
		attr('id', 'editwitop').
		addClass('headerbutton').
		val('Edit').
		click(_enterEditMode);

	var resetForm = function ()
	{
		if (getQueryStringVal("edit") && getQueryStringVal("query"))
		{
			redirLastQuerySession();
		}
		else
		{
			wiVform.reset();
			workController.form.reset();
			_statusDidChange();
			$('#newcomment').val();

			$('.relations tr').removeClass('deleted');

			stampsController.reset();
			workController.updateTimeLogged();

			_exitEditMode();
		}
	};

	var cancelButton = $(document.createElement('input')).
		attr('type', 'button').
		attr('id', 'canceledit').
		val('Cancel').
		click(resetForm).
		hide();
	var cancelButtonTop = $(document.createElement('input')).
		attr('type', 'button').
		attr('id', 'canceledittop').
		val('Cancel').
		click(resetForm).
		hide();

	var saveButton = wiVform.submitButton('Save Changes').
		hide();
	saveButton.click(_checkRedirect);

	var saveButtonTop = wiVform.submitButton('Save Changes').
		hide();
	saveButtonTop.click(_checkRedirect);

	wiSections.buttons.append(editButton);
	wiSections.buttons.append(saveButton);
	wiSections.buttons.append(cancelButton);

	wiSections.titleblock.append(editButtonTop);
	vvWiHistory.getToggle().insertAfter(editButtonTop);

	var bdiv = $(document.createElement('div')).
		attr('id', 'topbuttons').
		append(saveButtonTop).
		append(cancelButtonTop).
		prependTo(wiSections.titleblock).
		addClass("clearfix").
		hide();

	_editModeButtons = [cancelButton, saveButton, cancelButtonTop, saveButtonTop];
	_viewModeButtons = [editButton, editButtonTop];

	_statusDidChange();

	if (! recid || editMode == true)
	{
		_enterEditMode();
		$("#item_title").focus();
	}
	else
	{
		wiVform.disable();
		wiVform.removeSubForm(workController.form);
		workController.disable();
		stampsController.disable();
		_exitEditMode();
	}
}

function _fillInUsers()
{
	$.each(_userTexts,
		   function(i, el) {
			   el.text( _niceUser(el.text()) );
		   }
		  );
}

function _selectRecord(wit) {
	var newRec = ! wit.recid;

	var recid = '';
	if (! newRec)
		recid = wit.recid;

	$('#zfrecid').val(recid);
	_wirecid = recid;
	_wit = wit;

	var title = '';

	if (newRec)
	{
		title = 'New Work Item';
		$("#wiWorkForm").hide();
	}
	else
	{
		title = wit.id;

		if (wit.title)
			title += ": " + wit.title;

		wiVform.removeSubForm(workController.form);
	}

	vCore.setPageTitle(title);

	workController.setWit(wit);

	_postPopulate(wit);
}

function _initWorkItem()
{
	var recid = getQueryStringVal('recid');
	var wit = {};

	var after = function ()
	{
	    $("#wiLoading").hide();
	    _selectRecord(wit);
	   
	    vCore.showSidebar();
	};

	$('#backtolist').unbind("click");

	_spinUp('zingform');
	utils_savingNow("Loading Work Item Details...");

	var url = false;
	if (recid)
		{
			var params = [];

			lurl = sgCurrentRepoUrl + '/workitem-full/' + recid + '.json?';

			vCore.ajax(
				{
				    "url": lurl,
				    dataType: 'json',

				    complete: function (xhr, textStatus, errorThrown)
				    {
				        _spinDown('zingform');
				        utils_doneSaving("Loading Work Item Details...");
				    },

				    success: function (data, status, xhr)
				    {
				        checkUnload = true;
				        vCore.setSprints(data.milestones);
				        vCore.setUsers(data.users);
				        wiVform.sprints = data.milestones;

				        wiVform.setUsers(data.users);

				        workController.modalTimePopup.setUsers(data.users);

				        delete data.sprints;
				        delete data.users;

				        workController.work = data.work || [];
				        delete data.work;

				        stampsController.setStamps(data.stamps);
				        delete data.stamps;

				        var sprints = [];

				        wit = data;

				        _saveVisitedItem(wit.id);

				        wiVform.setRecord(wit);

				        wiVform.ready();

				        after(wit);
				    },
				    error: function ()
				    {
				        checkUnload = false;
				    }


				}
			);
		}
	else
	{
		$("#work_estimate_label").text("Estimate");
		_spinDown('zingform');
		wiVform.ready();

		after();
	}

}

function _addChunk(chunkid, container, caption)
{
	var ctr = _findElement(container);
	var chunk;

	if (caption)
	{
		chunk = $(document.createElement('fieldset'));
		chunk.append($(document.createElement('legend')).text(caption));
	}
	else
	{
		chunk = $(document.createElement('div'));
	}

	chunk.attr('id', chunkid);

	ctr.append(chunk);

	return(chunk);
}

function _statusDidChange(e)
{
	var el = $('#dupcontainer');
	var input = wiVform.getField("status");
	var val = input.val();

	workController.statusDidChange(val);

	if (val == 'duplicate')
	{
		el.show();
	}
	else
	{
		el.hide();
	}
}

function _createAttacher()
{
	var ctr = $('<div class="buttoncontainer"></div>');

	var show = $(document.createElement('input')).
		attr('type', 'button').
		val('Add').
		attr('id', 'addattachment').
		appendTo(ctr);

	var pop = new modalPopup
	(
		"#backgroundPopup", null,
		{
			onClose: function()
			{
				_enableButton(show);
			}
		}
	);

	show.click(
		function (e)
		{
			_disableButton(show);
			var adiv = $(document.createElement('div')).css("width", "400px");

			var chooser = $(document.createElement('input')).
			attr('type', 'file').
			attr('id', 'attachfile').
			attr('name', 'attachfile').
			attr('size', 45).
			css('width', '100%').
			appendTo(adiv);

			var buttondiv = $(document.createElement('div'));
			var cancel = $(document.createElement('input')).
			attr('type', 'button').
			css('float', 'right').
			val('Cancel');
			var go = $(document.createElement('input')).
			attr('type', 'button').
			attr('id', 'attachbutton').
			css('float', 'right').
			val('Attach');

			buttondiv.append(go);
			buttondiv.append(cancel);

			cancel.click(function (e)
			{
				this.ok = false;
				pop.close();
			});
			go.click(function (e)
			{
				_spinUp(adiv);
				this.ok = true;

				if (!chooser.val())
					return;

				var uploadurl = sgCurrentRepoUrl + "/workitem/" + _wit.recid + "/attachments?From=" + vCore.getOption('userName');

				if (! jQuery.handleError)
					jQuery.handleError = function() {};

				$.ajaxFileUpload(
				{
					"url": uploadurl,
					"dataType": "text",
					"fileElementId": 'attachfile',
					"secureuri": "false",
					"opener": window,
					"pop": pop,

					complete: function ()
					{
						this.opener._spinDown(adiv);
					},

					success: function (data, textStatus)
					{
						this.pop.close();

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

						document.location.reload();
					},

					error: function (data, status, e)
					{
						this.pop.close();
						this.opener.reportError("Unable to update at this time. Your attachment was not saved.", false);
					}
				});
			});

			_enableButton(cancel);
			_disableButton(go);

			chooser.change(function (e)
			{
				if ($(this).val() && $(this).val().length)
				{
					_enableButton(go);
				}
				else
				{
					_disableButton(go);
				}
			});
			var win = pop.create("Add a File Attachment", adiv, null, buttondiv);

			$('#maincontent').prepend(win);

			var pos = {
				'left': ($(window).width() - win.width()) / 2,
				'top': ($(window).height() - win.height()) / 2
			};
			pop.show(null, null, pos);

		});

	return (ctr);
}

function _niceDate(dateval)
{
	var result = '';

	if (dateval)
	{
		var d = new Date(parseInt(dateval));

		result = shortDateTime(d); // d.strftime("%Y/%m/%d %i:%M%p").toLowerCase();
	}

	return(result);
}

var wimd = null;

function _savingNow(msg)
{
	wimd = new modalPopup('none', true, { css_class: "savingpopup" });
	var outer = $(document.createElement('div')).
		addClass('spinning');
	var saving = $(document.createElement('p')).
		addClass('saving').
		text(msg + "…").
		appendTo(outer);
	var p = wimd.create("", outer).addClass('info');
	$('#maincontent').prepend(p);

	_disableFields();

	window.scrollTo(0,0);
	wimd.show();
}

function _doneSaving()
{
	_enableFields();

	if (wimd)
		wimd.close();

	wimd = false;
}

function _goToItem(opts)
{
    redirecturl = sgCurrentRepoUrl + "/workitem.html?recid=" + opts.recid + "&query=true";

	if (opts.save)
	{
		wiVform.submitForm();
	}
	else
	{
		window.location.href = redirecturl;
	}

	lock = false;

}

function _dirty()
{
	var dirty = wiVform.isDirty(true);

	return dirty;
}

function _checkRedirect()
{
	if (getQueryStringVal("edit") && getQueryStringVal("query"))
		redirecturl = _getLastQuerySession();
}

function _checkForUnaddedStamp(callback, args)
{
	if (stampsController.isDirty())
	{
		var saveCallback = function() {
			stampsController.addStamp($("#wiSearchInput").val());
			callback(args);
		}
		
		var noCallback = function() {
			$("#wiSearchInput").val("");
			callback(args);
		}
		
	    confirmDialog("Add pending stamp \"" + $("#wiSearchInput").val() +"\" ?", $("#stamps"), saveCallback, null, noCallback, null, [50, -35], null);
		return false;
	}
	
	return true;
}

function _navigateToItem(itemid)
{	
	if (!_checkForUnaddedStamp(_navigateToItem, itemid))
		return;
	
	//the check for editmode is there for a reason
	//don't take it out
	if (_dirty())
	{
	    navHandled = true;
	    confirmDialog("Save item?", $("#rightSidebar"), _goToItem, { "recid": itemid, "save": true }, _goToItem, { "recid": itemid, "save": false }, [50, -35], function () { navHandled = false; });
	}
	else
	{
		_goToItem({ "recid": itemid, "save": false });
	}
}

function _setUpNavigation(id)
{
	fromQuery = true;
	vCore.ajax({
	    url: sgCurrentRepoUrl + "/workitem/" + id + "/navigation.json",
	    success: function (data)
	    {
	        var prevRecid = data.prev;
	        var nextRecid = data.next;
	        var navDiv = $("<div>").addClass("wiNavigation");

	        if (nextRecid)
	        {
	            var aR = $('<a>').attr("href", sgCurrentRepoUrl + "/workitem.html?recid=" + nextRecid);

	            var imgR = $('<img>').attr({ "src": sgStaticLinkRoot + "/img/blue_next.png" });
	            aR.html(imgR);
	            _menuWorkItemHover.addHover(aR, nextRecid, 35);
	            if (!lock)
	            {
	                aR.click(function (e)
	                {
	                    if (e.which != 2)
	                    {
	                        e.preventDefault();
                            e.stopPropagation();
	                        lock = true;
	                        _menuWorkItemHover.hide();
	                        _navigateToItem(nextRecid);
	                    }
	                });
	            }
	            navDiv.append(aR);
	        }

	        var aBack = $('<a>').attr({ "href": "#", "title": "filter results list" });

	        aBack.click(function (e)
	        {
	            e.preventDefault();
	            e.stopPropagation();
	            _menuWorkItemHover.hide();
	            if (_dirty())
	            {
	                navHandled = true;
	                confirmDialog("Save item?", $("#rightSidebar"),
				   function ()
				   {
				       redirecturl = _getLastQuerySession();
				       wiVform.submitForm();

				   }, null,
				   function () { redirLastQuerySession(); }, null, [50, -35], function () { navHandled = false; });
	            }
	            else
	            {
	                redirLastQuerySession();
	            }
	        });
	        var imgBack = $('<img>').attr({ "src": sgStaticLinkRoot + "/img/blue_goto_list.png" });

	        aBack.html(imgBack);
	        navDiv.append(aBack);

	        if (prevRecid)
	        {
	            var aL = $('<a>').attr("href", sgCurrentRepoUrl + "/workitem.html?recid=" + prevRecid);

	            var imgL = $('<img>').attr({ "src": sgStaticLinkRoot + "/img/blue_previous.png" });
	            aL.html(imgL);
	            _menuWorkItemHover.addHover(aL, prevRecid, 35);
	            if (!lock)
	            {
	                aL.click(function (e)
	                {
	                    if (e.which != 2)
	                    {
	                        e.preventDefault();
	                        e.stopPropagation();
	                        lock = true;
	                        _menuWorkItemHover.hide();
	                        _navigateToItem(prevRecid);
	                    }
	                });
	            }
	            navDiv.append(aL);
	        }

	        _vvrightsidebar.prepend(navDiv);
	    }
	});

}

var commentController = function(form, container)
{
	this.form = form;
	this.container = container;
	this.commentCont = null;
	this.addButton = null;
	this.commentField = null;
};

extend(commentController, {

    draw: function ()
    {
        var self = this;
        this.commentCont = $("<div>").attr("id", "comments");
        this.container.append(this.commentCont);

        this.commentField = this.form.field(this.container, "textarea", "comment", {
            label: "Add a Comment",
            rows: 4,
            disableable: false,
            validators: {
                liveValidation: true,
                maxLength: 16384
            }
        });

        this.addButton = this.form.button("Add Comment").
			click(function () { self.addCommentClicked(); });
        wiSections.buttons.append(this.addButton);

		_enableOnInput(this.addButton, this.commentField.field);
        _disableButton(this.addButton);
    },

    renderComment: function (cmt)
    {
        var cmtid = cmt.recid || '';
        var hdr = $('<h3 class="innerheading"></h3>');

        if (cmtid)
            hdr.attr('id', cmtid);

        var d = new Date();
        d.setTime(cmt.when);

        var datestr = shortDateTime(d); // d.strftime("%Y/%m/%d %i:%M%p").toLowerCase();
        hdr.text(" commented on " + datestr);
        var who = $(document.createElement('span')).
			text(_niceUser(cmt.who));
        hdr.prepend(who);

        _userTexts.push(who);

        var t = $('<p class="commentbody innercontainer"></p>');
        var cont = vCore.createLinks(cmt.text);
        cont = cont.replace(/(\r\n|\n)/g, "<br />");
        t.html(cont);

        this.commentCont.append(hdr);

        hdr.after(t);

        if (cmtid && document.location.hash && (document.location.hash == ('#' + cmtid)))
            return (hdr);
        else
            return (null);
    },

    addCommentClicked: function (event)
    {
        var self = this;

		_disableButton( self.addButton );

        this.form.clearErrors();
        if (!this.form.validateInput(this.commentField, true))
        {
            this.form.displayErrors();
			_enableButton( self.addButton );
            return;
        }

        var data = {
            "text": this.commentField.val(),
            "when": (new Date()).getTime(),
            "who": sgUserName
        };

        var jdata = JSON.stringify(data);
        var recid = this.form.getRecordValue("recid");

        var lurl = sgCurrentRepoUrl + '/workitem/' + recid + '/comments.json';

        _spinUp(this.commentField);
        _savingNow("Adding your comment");

        vCore.ajax
		(
			{
			    url: lurl,
			    dataType: 'text',
			    contentType: 'application/json',
			    processData: false,
			    type: 'PUT',
			    data: jdata,

			    complete: function ()
			    {
			        if (document.body)
			        {
			            _doneSaving();
			            _spinDown(self.commentField);
			        }
			    },

			    success: function (rdata)
			    {
			        if (document.body)
					{
						self.renderComment(data);
			            self.commentField.setVal("", true);
			            vvWiHistory.reset();
					}
			    },

				error: function()
				{
					_enableButton( self.addButton );
				}
			});
    },

    enable: function ()
    {
        this.addButton.show();
    },

    disable: function ()
    {
        this.addButton.hide();
    }
});

function _toggleDescriptionView()
{
    var params = $.deparam.fragment();
    var descField = wiVform.getField("description");
    var div = $("#desc").find('.reader');
    if (params["plain"] == "true")    {
      
        div.addClass("raw");
        descField.options.reader.markedDown = false;
        descField.options.reader.formatter = null;
        $("#spanMarkdownSelect").text("Plain Text");
    }
    else
    {
        div.removeClass("raw");   
        descField.options.reader.markedDown = true;
        descField.options.reader.formatter = function (val) { return wiVform.markdowneditor.converter.makeHtml(val) };
        $("#spanMarkdownSelect").text("Markdown");
    }
    descField.setReaderVal(wiVform.getRecordValue("description"));

}
function _setupRawView()
{
    var rawDiv = $("<div>").attr("id", "divMarkdownView").text("View:");

    var rawSpan = $("<span>").attr("id", "spanMarkdownSelect").text("Markdown").addClass("spanMarkdown");
    rawDiv.append(rawSpan);
    var divMenu = $("<div>").addClass("markdownMenu menulink").attr("id", "divMarkdownMenu").hide();
    divMenu.hover(function ()
    {
        $(this).css("background-color", "#F6F8EF");
    },
    function ()
    {
        $(this).css("background-color", "#E6EDD4");
    });

    rawDiv.click(function (e)
    {
        if ($('#divMarkdownMenu').is(':visible'))
        {
            divMenu.hide();
        }
        else
        {
            if ($("#spanMarkdownSelect").text() == "Markdown")
            {
                divMenu.text("Plain Text");
            }
            else
            {
                divMenu.text("Markdown");
            }
            divMenu.css({ "top": $(this).position().top + 19, "left": $(this).position().left + 24});
            divMenu.show();
        }
    });
    divMenu.click(function (e)
    {
        if ($(this).text() == "Plain Text")
        {
            $.bbq.pushState({ "plain": true });
        }
        else
        {
            $.bbq.pushState({ "plain": false });
        }
        $(window).trigger('hashchange');     
        $(this).hide();

    });

    $(".labeldescreader").append(rawDiv);
  
    divMenu.insertBefore($("#item_wmd-inputdescription_label"));

    $(document).mouseup($(this), function (e)
    {
        if (!$(e.target).hasClass("markdownMenu") && !$(e.target).hasClass("divMarkdownView") && !$(e.target).hasClass("spanMarkdown"))
        {
            $(".markdownMenu").hide();
        }

    });
}

function hashStateChanged(e)
{
    _toggleDescriptionView();
}
function _moveToCredits(f)
{
	$('<label></label>').text(f.label.text()).appendTo(wiSections.credits);
	f.reader.appendTo(wiSections.credits);
}

function beforeSubmitCallback()
{	
	var callback = function() { wiVform.submitForm(); }
	if (!_checkForUnaddedStamp(callback))
		return false
}

function sgKickoff()
{
	_vvrightsidebar = $('#rightSidebar');
	if (getQueryStringVal("edit"))
	   editMode = true;
	if (getQueryStringVal("history"))
	   _showHistory = true;

	_changesetHover = new hoverControl(fetchChangesetHover);
	_vvrightsidebar.addClass('wit');

	var recid = getQueryStringVal('recid');

	if (recid)
		setCurrentSection('workitems');
	else
	{
		setCurrentSection('workitem');
		vvredirectable = 'workitem.html';
	}

	var opts = {
	    rectype: 'item',
	    prettyReader: true,
	    queuedSave: false,
	    afterSubmit: _reloadWorkItem,
	    errorFormatter: bubbleErrorFormatter,
		beforeSubmit: beforeSubmitCallback,
	    afterValidation: function () { window.onbeforeunload = null; }
	};

	if (recid)
	{
	   opts.url = sgCurrentRepoUrl + "/workitem-full/" + recid + ".json";

	   opts.method = "PUT";
	}
	else
	{
	   opts.url = sgCurrentRepoUrl + "/workitems.json";
	   opts.method = "POST";
	}

	opts.duringSubmit = {
	    start: function ()
	    {
	        navHandled = true;
	        _savingNow("Saving work item");
	    },

	    finish: function ()
	    {
	        navHandled = false;
	        _doneSaving();
	    }
	};

	wiVform = new vForm(opts);
	wiVform.field(_vvrightsidebar, "text", "id", {
		reader: { tag: 'h1', addClass: 'outerheading' },
		dontSubmit: true,
		readOnly: true
		});

	var container = $('<div class="outercontainer"></div>').appendTo(_vvrightsidebar);
	var creditHolder = $('<div class="showreadonly"></div>').appendTo(container);

	wiSections.people = _addChunk('people', container).addClass('hidereadonly');

	$('<div class="innerheading"></div>').appendTo(creditHolder);
	wiSections.credits = _addChunk('credits', creditHolder).addClass('witcredits innercontainer noadjust');

	wiSections.timetracking = _addChunk('timetracking', container);
	wiSections.stamps = _addChunk('stamps', container);

	$('<div class="innerheading">Stamps</div>').appendTo(wiSections.stamps);

	var fa = $('<div class="editable showreadonly"></div>').appendTo(container);
	$('<label class="innerheading">File Attachments</label>').appendTo(fa);
	wiSections.attachments = _addChunk('attachments', fa).
		addClass('innercontainer').
		css('min-height', '23px');
	wiSections.titleblock = $('#wiheader').addClass('clearfix');
	wiSections.desc = _addChunk('desc', 'zingform');

	var ctr = $('<div />').appendTo($('#zingform')).addClass('clearfix');
	wiSections.relations = _addChunk('relations', ctr).addClass('relctr relations emptydrop').hide();

	var d = $('<div class="relations"><h3 class="innerheading">Related Work Items</h3></div>').
		appendTo(wiSections.relations);

	var relctr = $('<div class="innercontainer"></div>').
		appendTo(d);

	relctr.append('<p class="dropplaceholder">Drag Work Items from the Activity Stream, or select from the Search box below.</p>');

	wiSections.csrelations = _addChunk('csrelations', ctr).addClass('relctr relations emptydrop');

	wiSections.comments = _addChunk('comments', 'zingform');
	wiSections.buttons = _addChunk('buttons', 'zingform');
	wiSections.sidebar = _vvrightsidebar;

	if (getQueryStringVal("query"))
	{
		fromQuery = true;
		_setUpNavigation(recid);
	}

	var md = null;

	wiVform.releasedSprintsOnly = !recid;  /* new bugs go into live sprints */

	$('<label class="innerheading">Reported by</label>').appendTo(wiSections.credits);
	var repcredit = $('<p class="reader innercontainer"></p>').appendTo(wiSections.credits);
    
	var f = wiVform.field(wiSections.people, "userSelect", "reporter", 
				  { 
					  readOnly: true, label: "Reported By", deferLoading: true,
					  onchange: function(input) {
						  repcredit.text(input.find("option:selected").text());
					  }
				  }
				 );

	f.label.addClass('neveredit');

	wiVform.field(wiSections.credits, "text", "created_date", { readOnly: true, dontSubmit: true, label: "On", reader: { formatter: _niceDate }});

	var f = wiVform.field(wiSections.people, "userSelect", "assignee", {
		label: "Assigned To",
		allowEmptySelection: true,
		deferLoading: true,
		wrapField: true
	});

	_moveToCredits(f);

	f = wiVform.field(wiSections.people, "userSelect", "verifier", {
		label: "Verifier",
		allowEmptySelection: true,
		deferLoading: true,
		wrapField: true
	});

	_moveToCredits(f);

	workController = new workFormController(
		{
			afterUpdate: function()
			{
				if (vvWiHistory)
				{
					vvWiHistory.reset();
					if (vvWiHistory.showing)
						vvWiHistory.show();
				}

			}
		}
	);

	wiSections.timetracking.append(workController.render());

	_moveToCredits( workController.form.getField('estimate') );
	wiVform.addSubForm(workController.form);

	stampsController = new stampsController(wiVform);
	wiSections.stamps.append(stampsController.render());

	var nameField = wiVform.field(wiSections.titleblock, "textarea", "title", {
		reader: {
			tag: 'p',
			hideLabel: true
		},
		label: "Name",
		multiLine: true,
		rows: 2,
		validators: {
			liveValidation: true,
			required: true,
			maxLength: 140
		}
	});

	nameField.field.css('display', 'block').css(
		{
			'-moz-box-sizing': 'border-box',
			'box-sizing': 'border-box',
			'width': '100%',
			'max-width': '100%'
		}
	);

	var statblock = $(document.createElement('div')).addClass('wimeta firstwimeta').attr('id', 'statblock');
	var sprintblock = $(document.createElement('div')).addClass('wimeta');
	var priorityblock = $(document.createElement('div')).addClass('wimeta');

	wiSections.titleblock.append(statblock).
		append(sprintblock).
		append(priorityblock);

	wiVform.hiddenField('_csid').
		appendTo(wiSections.titleblock);

	var status = wiVform.field(statblock, "select", "status", {
		label: "Status",
		reader: { tag: "span", hideLabel: true },
		allowEmptySelection: false,	
		validators: { required: true },
		onchange: _statusDidChange,
		wrapField: true,
		isBlock: true
	});

	status.addOption("open", "Open");
	status.addOption("fixed", "Fixed");
	status.addOption("verified", "Verified");
	status.addOption("wontfix", "Won't Fix");
	status.addOption("invalid", "Invalid");
	status.addOption("duplicate", "Duplicate");

	var milestoneField = wiVform.field(sprintblock, "sprintSelect", "milestone", {
	    reader: {
	        hideLabel: true,
	        tag: "span"
	    },
	    nested: true,
	    label: "Milestone",
	    emptySelectionText: "Product Backlog",
	    filter: function (val)
	    {
	        if (val)
	        {
	            if (val.recid == wiVform.record.milestone)
	                return true;
	            else
	                return val.releasedate == undefined;
	        }
	        else
	            return true;
	    },
	    validators: {
	        custom: function (value)
	        {
	            if ($("#item_status").val() == "open" && $("#item_milestone").find(":selected").data("released") == true)
	            {
	                return "Please choose a milestone that has not been released.";
	            }
	            return true;

	        }
	    },
	    wrapField: true,
	    isBlock: true
	});

	var priority = wiVform.field(priorityblock, "select", "priority", {
		label: "Priority",
		reader: { tag: "span", hideLabel: true },
		allowEmptySelection: true,
		emptySelectionText: '(Unassigned Priority)',
		wrapField: true,
		isBlock: true
	});

	priority.addOption('High', 'High Priority');
	priority.addOption('Medium', 'Medium Priority');
	priority.addOption('Low', 'Low Priority');

	priority.label.addClass("editlabel");
	status.label.addClass("editlabel");
	milestoneField.label.addClass("editlabel");

	var descField = wiVform.field(wiSections.desc, "markdowntextarea", "description", {
	    reader: {tag: "div"},
		label: 'Description',
		multiline: true,
		rows: 10,
		addclass: "overflow-auto",
		validators: { maxLength: 16384, liveValidation: true }
    });
          
	wiCommentController = new commentController(wiVform, wiSections.comments);
	wiCommentController.draw();

	var att = _createAttacher().
		insertAfter(wiSections.attachments);
	$('<div class="clearfix"></div>').insertAfter(att);
	
	vvWiHistory.wrap('#zingform');

	if (_showHistory)
	   vvWiHistory.show();

	wiVform.disable();

	_initWorkItem();

	_setupRawView();
	
	
	//_vvmarkdown.addEditor(wiSections.desc, descField.field);
	_addAdder('workitem.html');
	var params = $.deparam.fragment();
	if (params["raw"] == "true")
	{
	    $("#checkboxRaw").attr("checked", "checked");
	}
	_toggleDescriptionView();

	$(window).bind('hashchange', hashStateChanged);
	window.onbeforeunload = null;
	window.onbeforeunload = function ()
	{
	    if (checkUnload)
	    {
	        if (_dirty() && !navHandled)
	            return "You have unsaved changes.";
	    }
	};
    
}
