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

var activityTries = 0;
var _actCsHover = null;
var _actWiHover = null;
var _actShowState = null;
var _activityPending = 0;
var _lastActivityETag = "";

var _vvact = {
	_first: true,

	spinOnce: function()
	{
		if (this._first)
		{
			_actPager.init('#activitylist', '#activitynav');

			this.spin();
			this._first = false;
			return true;
		}
		return false;
	},

	unspin: function()
	{
		$('#imgrefreshact').attr('src', sgStaticLinkRoot + "/img/stream_refresh.png");
	},

	spin: function()
	{
		$('#imgrefreshact').attr('src', sgStaticLinkRoot + "/img/smallBlueProgress.gif");
	}
};

function activityItem(activity) {
	var li = $("<li>");

	var p = $("<p>").addClass('act');
	var a = null;

	for ( var f in activity )
	{
		if (typeof(activity[f]) == "string")
			activity[f] = vCore.sanitize(activity[f]);
	}

	var hasLink = false;
	if ((!!activity.link) && ($.trim(activity.link) != ""))
	{
		var ln = sgLinkRoot;
		var repurl = null;
		var repo = startRepo;

		var matches = activity.link.match(/^(\/repos\/.+?)\//);

		if (matches)
		{
			var r = matches[1];
			repurl = sgLinkRoot + r;

			matches = r.match(/repos\/(.+)/);

			if (matches)
				repo = matches[1];
		}
		else
			ln += '/repos/' + encodeURIComponent(startRepo);

		ln += activity.link;

		a = $("<a>").attr("href", ln);
		hasLink = true;

		matches = activity.link.match(/\/(workitem|changeset).html\?recid=([a-z0-9]+)/);

		if (matches)
		{
			var thing = matches[2];
			var type = matches[1];

			var rels = $('#relations') || $('#wmd-input');
			var field = 'other';
			var id = null;

			if (type == 'changeset')
				_actCsHover.addHover(a, thing, null, null, repurl);
			else
				_actWiHover.addHover(a, thing);

			if (type == 'changeset')
			{
				rels = $('#csrelations') || $('#wmd-input');
				field = 'commit';
			}
			else
			{
				matches = activity.action.match(/([0-9A-Z]+)$/);

				if (matches)
					id = matches[1];
			}

			if (rels)
			{
				li.draggable(
					{
						disabled: ! window.editMode,
						helper: 'clone'
					}
				);

				var rel = { 'id': id };
				rel[field] = thing;
				rel.repo = repo;
				li.data('rel', rel );
			}
		}
	}

	if (activity.action != undefined) {
		var act_txt = activity.action + ": ";
		if (hasLink) {
			a.text(act_txt);
			p.append(a);
		}
		else
		{
			p.append(act_txt);
		}
	}

	var el = null;

	if (hasLink && (activity.action == undefined))
	{
		a.text(activity.what);
		el = a;
	}
	else
	{
		var sp = $("<span />").text(activity.what);
		el = sp;
	}

	el.html(vCore.createLinks($.trim(el.text())));
	el.html(el.html().replace(/(\r\n|\n)/g, "<br />"));
	el.addClass('actcontent');
	p.append(el);

	li.append(p);

	p = $("<p>");
	p.addClass('byline');

	var d = new Date(parseInt(activity.when));
	var who = activity.email || activity.who || '';
	p.text(_shortWho(who));
	p.append($('<br>'));

	p.append(shortDateTime(d));

	li.append(p);

	return (li);
}


function unqueueActivity()
{
	if (window._activityPending > 0)
		--window._activityPending;

	listActivity();
}

function queueActivity(timeout) {
	if (! timeout)
		timeout = 60000;

	if (window._activityPending > 0)
		return;

	window._activityPending = 1;
	window.setTimeout(unqueueActivity, timeout);
}


function listActivity(callback) {
	if (callback && (typeof(callback) != 'function'))
		callback = null;

	if (!_showActivity()) {
		if (callback)
			callback();
		queueActivity();
		return;
	}

	if (vCore.ajaxCallCount > 0)
	{
		++activityTries;

		if (activityTries < 60)
		{
			if (callback)
				callback();
			queueActivity(500);
			return;
		}
	}

	activityTries = 0;

	var listUrl = sgCurrentRepoUrl + "/activity.json";

	if (_filterActivity() && window.sgUserName)
	{
		listUrl = sgCurrentRepoUrl + "/activity/" + sgUserName + ".json";
		var actXml = vCore.getOption('useractfeed') || (sgCurrentRepoUrl_universal + "/activity/" + sgUserName + ".xml");
		$('#linkactfeed').attr('href', actXml);
	}
	else
	{
		var actXml = vCore.getOption('actfeed') || (sgCurrentRepoUrl_universal + "/activity.xml");
		$('#linkactfeed').attr('href', actXml);
	}

	var first = _vvact.spinOnce();
	setActivityHeight();

	vCore.ajax( {
		url : listUrl,
		dataType : 'json',
		cache : true,
		reportErrors: false,
		redirectOnAuthFail: false,

		beforeSend: function(xhr)
		{
			if (!first)
				xhr.setRequestHeader("If-None-Match", _lastActivityETag);
		},

		complete : function(xhr, textStatus) {
			_vvact.unspin();

			setActivityHeight();

			queueActivity();
			if (callback)
				callback();
		},

		success : function(data, status, xhr) {
			if ((status != "success") || (xhr.status === 304) || (!data)) {
				return;
			}

			_lastActivityETag = xhr.getResponseHeader("ETag");

			var uls = $('#activitylist ul');
			var ul = false;

			if (uls.length > 0) {
				ul = $(uls[0]);
			} else {
				ul = $(document.createElement("ul"));
				$('#activitylist').append(ul);
			}

			ul.empty();

			$.each(data, function(index, activity) {
				ul.append( activityItem(activity) );
			});

			_actPager.init('#activitylist', '#activitynav');
		}
	});
}

var _actPager = {
	box : false,
	curTop : 0,
	_ready: false,

	init : function(ctr, nav) {
		this.box = $(ctr);
		this.nav = $(nav);
		this.box.attr('overflow', 'hidden');
		this.alist = $($(ctr + ' ul')[0]);

		var prevButton = $('#prevActivity');
		var nextButton = $('#nextActivity');

		if (! this._ready)
		{
			prevButton.click
			(
				function(e)
				{
					e.stopPropagation();
					e.preventDefault();
					_actPager.backUp();
				}
			);

			nextButton.click
			(
				function(e)
				{
					e.stopPropagation();
					e.preventDefault();

					_actPager.curTop = Math.max(nextButton.data('hidingAfter') || 0, 0);
					_actPager.redisplay();
				}
			);

			this._ready = true;
		}

		this.redisplay();
	},

	removeLastWordOrNode: function(el) {
		var kids = el.contents();
		if (kids.length == 0)
			return false;

		var last = kids.last();

		if (last[0].nodeType == 3)   // text node
		{
			var txt = last.text();
			var re = /^(.+)\s+(\S)$/;

			var matches = txt.match(re);

			if (matches)
			{
				txt = matches[1];
				last.text(txt);
			}
			else
			{
				last.remove();
			}
		}
		else
			last.remove();

		return true;
	},

	redisplay : function() {
		if (! this.box)
			return;

		var top = this.box.offset().top;
		var scrollTop = $(window).scrollTop();
		var wheight = windowHeight();
		var bheight = wheight - top - 40 + scrollTop;
		var hidingAfter = -1;
		var prevButton = $('#prevActivity');
		var nextButton = $('#nextActivity');

		this.box.height(bheight);

		var els = this.alist.children('li');
		if (this.curTop >= (els.length))
			this.curTop = 0;

		var first = true;
		var self = this;

		els.each(
			function(idx, liv)
			{
				var li = $(liv);
				if ((idx < _actPager.curTop) || (hidingAfter >= 0))
				{
					li.hide();
				}
				else
				{
					li.show();

					var done = ! first;

					do
					{
						var bottom = li.offset().top + li.height() - top;

						if (bottom >= bheight) 
						{
							if (first)
							{
								if (! self.removeLastWordOrNode(li.find('.actcontent')))
									done = true;
							}
							else
							{
								li.hide();

								if (hidingAfter < 0)
									hidingAfter = idx;
							}
						}
						else
						{
							done = true;
						}
					} while (! done);

					first = false;
				}
			});

		if (this.curTop > 0)
		{
			prevButton.removeAttr('disabled');
			prevButton.find('img').show();
		}
		else
		{
			prevButton.attr('disabled', 'disabled');
			prevButton.find('img').hide();
		}

		if (hidingAfter >= 0)
		{
			nextButton.removeAttr('disabled');
			nextButton.find('img').show();
		}
		else
		{
			nextButton.attr('disabled', 'disabled');
			nextButton.find('img').hide();
		}

		nextButton.data('hidingAfter', hidingAfter);

		//var hidden = ! this.nav.is(':visible');
		this.nav.show();

	},

	backUp : function() {
		var top = this.box.offset().top;
		var wheight = windowHeight();
		var bheight = wheight - top - 30;
		var hidingAfter = -1;

		this.box.height(bheight);

		var els = this.alist.children('li');

		var i = this.curTop - 1;
		var canary = $(els[i]);

		this.curTop = 0;

		while (i > 0) {
			var testee = els[i];
			$(testee).show();

			var bottom = canary.offset().top + canary.height() - top;

			if (bottom >= bheight) {
				this.curTop = i + 1;
				break;
			}

			--i;
		}

		this.redisplay();
	}
};

function _setShowActivity(show) {
	var cname = vCore.getGlobalDefaultCookieName();
	vCore.setCookieValueForKey(cname, "showActivity", show, 30);

	sgShowActivity = show;

	if (show)
	{
		$('#menushowactivity').addClass("checked");
		$('#menufilteractivity').show();
	}
	else
	{
		$('#menushowactivity').removeClass("checked");
		$('#menufilteractivity').hide();
	}

	_actShowState = show;
}

function _showActivity() {
	if (_actShowState === null)
	{
		var cname = vCore.getGlobalDefaultCookieName();
		var show = vCore.getCookieValueForKey(cname, "showActivity", false);
		_actShowState = (show && (show != "false")); // cookies stringify
	}

	return( _actShowState );
}

function _filterActivity(setfilter)
{
	var cname = vCore.getGlobalDefaultCookieName();

	if (setfilter !== undefined)
	{
		vCore.setCookieValueForKey(cname, "filterActivity", setfilter, 30);

		if (setfilter)
			$('#menufilteractivity').addClass("checked");
		else
			$('#menufilteractivity').removeClass("checked");
	}

	var v = vCore.getCookieValueForKey(cname, "filterActivity", false);

	return(v && (v != 'false'));
}

function hideActivity()
{
	$('#activity').width(0);
	$('#activity').hide();
	$('#activity').trigger('resize');
	$('#activity').trigger('repaintGraph');
	_actShowState = false;
}

function displayActivity()
{
	$('#activity').width(213);
	$('#activity').show();
	$('#activity').trigger('resize');
	$('#activity').trigger('repaintGraph');
	_actShowState = true;
}

function setActivityHeight()
{
	var wHeight = windowHeight();
	var top = parseInt($("#activity").css('top'));
	var actHeight = wHeight - top;

	// Make sure the size actually changes so we aren't
	// sending an unnecessary resize event (ahem... IE)
	if (actHeight != $("#activity").height())
	{
		actHeight = actHeight + "px";
		$("#activity").css( { height:actHeight } );
	}
}

$(document).ready(function() {
	setActivityHeight();

	$(window).resize(function() {
		setActivityHeight();
		_actPager.redisplay();
	});

    $("#linkrefreshact").click(function (e)
    {
        e.preventDefault();
        e.stopPropagation();

        _vvact.spin();
        listActivity();
    });

    _actCsHover = new hoverControl(fetchChangesetHover);
    _actWiHover = new hoverControl(fetchWorkItemHover);

    if (_showActivity() && !sgUserName)
        _setShowActivity(false);

    if (_showActivity())
    {
        displayActivity();
        queueActivity(1000);
    } else
    {
        hideActivity();
    }

});
