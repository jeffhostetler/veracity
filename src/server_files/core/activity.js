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

registerRoutes({
	"/repos/<repoName>/activity.json": {
		"GET": {
			onDispatch: function (request) {
				sg.server.request_profiler_start(sg.server.ACTIVITY_DISPATCH);
				try{
					var cc = new textCache(request, activityStreams.getDags());
	
					var inm = request.headers["If-None-Match"];
					if (inm !== undefined && inm !== null && inm.length !== 0)
						return (cc.get_possible_304_response(activityStreams.getActivity, activityStreams, request));
					else
						return (cc.get_response(activityStreams.getActivity, activityStreams, request));
				}
				finally{
					sg.server.request_profiler_stop();
				}
			}
		}
	},

	"/repos/<repoName>/activity/<userid>.json": {
		"GET": {
			onDispatch: function (request) {
				sg.server.request_profiler_start(sg.server.ACTIVITY_DISPATCH);
				try{
					var cc = new textCache(request, activityStreams.getDags());
	
					var inm = request.headers["If-None-Match"];
					if (inm !== undefined && inm !== null && inm.length !== 0)
						return (cc.get_possible_304_response(activityStreams.getActivity, activityStreams, request, request.userid));
					else
						return (cc.get_response(activityStreams.getActivity, activityStreams, request, request.userid));
				}
				finally{
					sg.server.request_profiler_stop();
				}
			}
		}
	},

	"/repos/<repoName>/activity.xml": {
		"GET": {
			onDispatch: function (request) {
				sg.server.request_profiler_start(sg.server.ACTIVITY_DISPATCH);
				try{
					var cc = new textCache(request, activityStreams.getDags(), CONTENT_TYPE__ATOM);
					return (cc.get_possible_304_response(activityStreams.getActivityAtom, activityStreams, request));
				}
				finally{
					sg.server.request_profiler_stop();
				}
			}
		}
	},

	"/repos/<repoName>/activity/<userid>.xml": {
		"GET": {
			onDispatch: function (request) {
				sg.server.request_profiler_start(sg.server.ACTIVITY_DISPATCH);
				try{
					var cc = new textCache(request, activityStreams.getDags(), CONTENT_TYPE__ATOM);
					return (cc.get_possible_304_response(activityStreams.getActivityAtom, activityStreams, request, request.userid));
				}
				finally{
					sg.server.request_profiler_stop();
				}
			}
		}
	}
});


var activityStreams =
{
	sources: [],

	/**
	 * activityStreams.addSource() registers an additional activity source
	 *
	 * an activitySource is an object implementing the interface:
	 *
	 * source.name():  (optional)
	 *   describes this source. for debugging only.
	 *
	 * source.dagsUsed():
	 *   and array of dags this activity stream cares about
	 *   e.g. [ sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL ]
	 *
	 * source.getActivity(request, max, userid):
	 *   return the most recent (up to) max records for this activity source.
	 *   userid is optional, and sources may ignore it. If present, it may be
	 *   used to trim the activities down to those relevant to that user id.
	 *
	 *   each record contains the following
	 *     .action  (optional - e.g. "Checkin", "P00033 Updated")
	 *     .link  (optional - e.g. /workitem.html?recid=....)
	 *            ( .../repo/reponame/ is implied)
	 *     .what  (e.g. "repolarized the fetzer valve")
	 *     .who   (user recid)
	 *     .when  (millisecond timestamp)
	 *
	 *   note that we don't care about the order in which the records are returned; they'll
	 *   be sorted afterwards by the caller
	 */
	addSource: function(activitySource)
	{
		this.sources.push(activitySource);
	},

	getDags: function()
	{
		var dags = {};
		var results = [];

		vv.each(this.sources,
				function(i, source)
				{
					var sourcedags = source.dagsUsed();

					vv.each(sourcedags,
							function(j, sourcedag)
							{
								if (! dags[sourcedag])
								{
									dags[sourcedag] = true;
									results.push(sourcedag);
								}
							}
						   );
				}
			   );

		results.sort();

		return(results);
	},

	getActivity: function(request, userid)
	{
		if (!request.repo)
			return(null);

		var allRecs = [];

		var max = 25;

		var uid = userid || "";

		var req = {
			requestMethod: "get",
			uri: "activity",
			headers: {},
			repo: request.repo
		};
		req.repoName = request.repoName;

		seen = [];
		vv.each(this.sources,
				function(ii, source)
				{
					if (source.module && ! moduleEnabled(source.module))
						return;

					req.queryString = source.name() + "/" + uid;

					var cc = new textCache(req, source.dagsUsed());

					try
					{
						var recs = cc.get_obj(source.getActivity, source, request, max, userid);

						for (var i = 0; i < recs.length; i++)
						{
							var rec = recs[i];
							var hash = sg.hash(sg.to_json(rec));
							if (seen.indexOf(hash) == -1)
							{
								seen.push(hash);
								allRecs.push(rec);
							}
						}
					}
					catch (ex) 
					{
						sg.log("urgent", "error retrieving activity from " + source.name() + ": " + ex);
					}
				}
				);

		vv.each(allRecs,
				function(j, rec)
				{
					if (rec.who && vv.lookupUser(rec.who, request.repo))
						rec.email = vv.lookupUser(rec.who, request.repo);

					if (rec.what && (rec.what.length > 512))
						rec.what = rec.what.substring(0, 512) + "...";
				}
				);

		allRecs.sort(
			function(a,b)
			{
				var adate = a.sortDate || a.when;
				var bdate = b.sortDate || b.when;

				var res = bdate - adate;

				if (res == 0)
					if ((a.action == "commit") && (b.action == "commit") && a.revno && b.revno)
						res = b.revno - a.revno;

				if ((res == 0) && (a._csid || b._csid))
				{
					var acsid = a._csid || "";

					res = acsid.localeCompare(b._csid || "");
				}
				if ((res == 0) && (a._bugid || b._bugid))
				{
					var abugid = a._bugid || "";

					res = abugid.localeCompare(b._bugid || "");
				}

				return( res );
			}
		);

		for ( var i = allRecs.length - 1; i > 0; --i )
		{
			var cmt = -1;
			var stat = -1;
			var mod = -1;
			var j = i - 1;
			if (allRecs[i]._acttype == 'bugcomment')
				cmt = i;
			else if (allRecs[i]._acttype == 'bugstatus')
				stat = i;
			else if (allRecs[i]._acttype == 'bugmod')
				mod = i;
			if (allRecs[j]._acttype == 'bugcomment')
				cmt = j;
			else if (allRecs[j]._acttype == 'bugstatus')
				stat = j;
			else if (allRecs[j]._acttype == 'bugmod')
				mod = j;

			if (cmt < 0)
				continue;
			else if ((mod < 0) && (stat < 0))
				continue;

			var against = (mod >= 0) ? mod : stat;
			if (allRecs[cmt]._csid != allRecs[against]._csid)
				continue;

			if (allRecs[cmt]._bugid != allRecs[against]._bugid)
				continue;

			if (stat >= 0)
			{
				if (allRecs[stat].action.match(/^Modified /))
					allRecs[j].what = allRecs[stat].what + "\nComment: " + allRecs[cmt].what;
				else
					allRecs[j].what = allRecs[cmt].what;

				allRecs[j].link = allRecs[cmt].link;
				allRecs[j].action = allRecs[stat].action;
			}
			else
			{
				allRecs[j].action = allRecs[mod].action;
				allRecs[j].link = allRecs[cmt].link;
				allRecs[j].what = allRecs[mod].what + "\nComment: " + allRecs[cmt].what;
			}

			allRecs.splice(i, 1);
		}

		if (allRecs.length > max)
			allRecs.splice(max, allRecs.length - max);

		return(allRecs);
	},

	_pad: function(ds, digits) {
		digits = digits || 2;

		if (ds < 10)
			ds = "0" + ds;

		if (digits == 3)
			if (ds < 100)
				ds = "0" + ds;

		return(ds);
	},

	_genId: function(repoName, hostname, link, who, when) {
		var linkstr;

		var host = hostname.replace(/:.*/, '');
		host = host.replace(/\.+$/, '');

		var d = new Date(when);

		if (link)
			linkstr = repoName + link;
		else
			linkstr = "/" + repoName + "/" + who +
			d.getUTCFullYear() + this._pad(d.getUTCMonth() + 1) + this._pad(d.getUTCDate()) +
			this._pad(d.getUTCHours()) + this._pad(d.getUTCMinutes()) + this._pad(d.getUTCSeconds()) + this._pad(d.getUTCMilliseconds(), 3);

		var id = "tag:" + host + "," +
			d.getUTCFullYear() + "-" + this._pad(d.getUTCMonth() + 1) + "-" + this._pad(d.getUTCDate()) + ":" +
			when + "/" + linkstr;

		return(id);
	},

	_atomTime: function(ms) {
		var d = new Date(ms);

		var timestr = d.getUTCFullYear() + "-" +
			this._pad(d.getUTCMonth() + 1) + "-" +
			this._pad(d.getUTCDate()) + "T" +
			this._pad(d.getUTCHours()) + ":" +
			this._pad(d.getUTCMinutes()) + ":" +
			this._pad(d.getUTCSeconds()) + "." +
			this._pad(d.getUTCMilliseconds(), 3) + "Z";

		return(timestr);
	},

	getActivityAtom: function(request, userid) {
		var activity = this.getActivity(request, userid);
		var maxwhen = 0;
		var username = false;

		for ( var i in activity )
		{
			var when = parseInt(activity[i].when);

			if (when > maxwhen)
				maxwhen = when;
		}

		if (userid)
		{
			if (request.repo)
			{
				username = vv.lookupUser(userid, request.repo);
			}
		}

		var baseUrl = request.linkRoot + "/repos/" + encodeURIComponent(request.repoName);

		var w = new xmlwriter();
		w.start_element("feed");
		w.attribute("xmlns", "http://www.w3.org/2005/Atom");

		var title = "activity in " + request.repoName;

		if (username)
			title = username + "'s " + title;

		w.start_element("title");
		w.content("veracity: " + title);
		w.end_element();

		w.start_element("author");
		w.element("name", "Veracity");
		w.element("email", "veracity@example.com");
		w.end_element();

		var selflink = baseUrl + "/activity";

		if (userid)
			selflink += "/" + userid;

		selflink += ".xml";

		w.start_element("link");
		w.attribute("rel", "self");
		w.attribute("href", selflink);
		w.attribute("type", "application/atom+xml");
		w.end_element();

		w.element("id",selflink);

		var timestr = this._atomTime(maxwhen);

		w.element("updated", timestr);

		for ( i in activity )
		{
			var act = activity[i];

			var linkstr = false;

			if (act.link.match(/^\/repos\//))
				linkstr = request.linkRoot + act.link;
			else if (act.link)
				linkstr = baseUrl + act.link;

			var id = this._genId(request.repoName, request.hostname, act.link, act.email || act.who, parseInt(act.when));

			w.start_element("entry");
			w.element("id", id);
			w.start_element("author");
			w.element("email", act.email || act.who);
			w.element("name", act.email || act.who);
			w.end_element();

			title = act.title || act.what;

			if (act.action)
				title = act.action + ": " + title;
			w.element("title", title);
			w.element("summary", act.what);

			if (linkstr)
			{
				w.start_element("link");

				w.attribute("rel", "alternate");
				w.attribute("type", "text/html");
				w.attribute("href", linkstr);

				w.end_element();
			}

			w.start_element("content");
			w.attribute("type", "text");
			w.content(act.atomContent || act.description || act.what);
			w.end_element();

			w.element("updated", this._atomTime(parseInt(act.when)));

			w.end_element();
		}

		w.end_element();

		return( w.finish() );
	}
};


var commitSource = {
	name: function()
	{
		return("commitSource");
	},

	dagsUsed: function()
	{
		return( [sg.dagnum.VERSION_CONTROL, sg.dagnum.VC_COMMENTS, sg.dagnum.VC_BRANCHES ] );
	},

	getHistoryAndBranches: function(repo, max)
	{
		max = max || 25;
		var sofar = 0;
		var skip = 0;

		while (sofar < max)
		{
			var history = repo.history(skip + max, null, null, null, null, null, null, false);

			if ((! history) || (! history.items) || (history.items.length <= skip))
				return;

			history = history.items;

			for ( var i = skip; i < history.length; ++i )
			{
				var changeset = history[i];

				if (! changeset.audits)
					continue;

				var csid = changeset.changeset_id;

				if (! this.cslist[csid])
				{
					var record = {
						action: "commit",
						link: "/changeset.html?recid=" + changeset.changeset_id,
						who: changeset.audits[0].userid,
						when: changeset.audits[0].timestamp,
						revno: changeset.revno || null,
						what: "",
						branches: [],
						parents: changeset.parents
					};

					if (changeset.comments && changeset.comments.length > 0)
					{
						record.what = changeset.comments[0].text;
						this.cmtHidsSeen[ csid + ":" + changeset.comments[0].hidrec ] = true;
					}

 					this.cslist[csid] = record;

					if (! record.what.match(/^[ \t]*me?rge?[ \t]*\.?[ \t]*$/i))
						sofar++;
				}
			}

			skip += history.length;
		}
	},

	_propagateBranch: function(branchName, branchTree, csid)
	{
		if (this.cslist[csid].branches.indexOf(branchName) < 0)
			this.cslist[csid].branches.push(branchName);
		branchTree[csid].branches[branchName] = true;
	},

	addBranches: function(repo)
	{
		var csids = [];
		var branchTree = {};

		for ( var csid in this.cslist )
		{
			branchTree[csid] = {
				"branches": {},
				"parents": this.cslist[csid].parents
			};
			csids.push(csid);
		}

		var closed = repo.named_branches().closed;

		for ( var branch in closed )
			closed[branch] = true;

		var initial = repo.lookup_branch_names({"changesets":csids});

		for ( csid in initial )
		{
			for ( var bn in initial[csid] )
				if (! closed[bn])
					this._propagateBranch(bn, branchTree, csid);
		}
	},

	_pad: function(i)
	{
		var res = "" + i;

		while (res.length < 2)
			res = "0" + res;

		return(res);
	},

	_dstr: function(dateval)
	{
		var d = new Date(parseInt(dateval));

		var st = d.getFullYear() + "-" + this._pad(d.getMonth() + 1) + "-" + this._pad(d.getDate()) + " " +
			this._pad(d.getHours()) + ":" + this._pad(d.getMinutes()) + ":" + this._pad(d.getSeconds());

		return(st);
	},

	getActivity: function(request, max, userid)
	{
		this.cslist = {};
		this.cmtHidsSeen = {};

		this.getHistoryAndBranches(request.repo, max);

		this.addBranches(request.repo);

		var results = [];

		// blow out this.cslist to array
		for ( csid in this.cslist )
		{
			var rec = this.cslist[csid];

			if (rec.what.match(/^[ \t]*me?rge?[ \t]*\.?[ \t]*$/i))
				continue;

			if (rec.branches.length > 0)
				rec.what += " (" + rec.branches.join(', ') + ")";

			results.push(rec);
		}

		results.sort(
			function(a,b) {
				return( b.revno - a.revno );
			}
		);

		var lastDate = 0;

		if (results.length > 0)
		{
			lastDate = results[0].when;

			for ( var i = 1; i < results.length; ++i )
			{
				if (results[i].when > lastDate)
					results[i].sortDate = lastDate;
				else
					results[i].sortDate = results[i].when;

				lastDate = results[i].sortDate;
			}

			for ( i = results.length - 2; i >= 0; --i )
			{
				if (results[i].when < lastDate)
					results[i].sortDate = lastDate;
				else
					results[i].sortDate = results[i].when;

				lastDate = results[i].sortDate;
			}
		}

		// turned off until performance issues with the comment history query are resolved
		// this.addCommentActivity(request.repo, results, this.cmtHidsSeen, max);

		return(results);
	},

	addCommentActivity: function(repo, results, cmtHidsSeen, max)
	{
		var db = new zingdb(repo, sg.dagnum.VC_COMMENTS);

		var recs = db.query('item', ['csid','text','history','last_timestamp'], null, 'last_timestamp #DESC', max);

		for ( var i = 0; i < recs.length; ++i )
		{
			var cmt = recs[i];

			if ((! cmt.history) || (cmt.history.length == 0))
				continue;

			var hist = vv.getFirstHistory(cmt);

			if (cmtHidsSeen[cmt.csid + ":" + hist.hidrec])
			{
				continue;
			}

			var audit = vv.getFirstAudit(cmt);

			var record = {
				action: "changeset comment added",
				link: "/changeset.html?recid=" + cmt.csid,
				who: audit.userid,
				when: audit.timestamp,
				what: cmt.text
			};

			try
			{
				var desc = repo.get_changeset_description(cmt.csid);
				record.revno = desc.revno;
			}
			catch (e)  // cs is not here yet
			{
				continue;
			}

			results.push(record);
		}
	}
};


activityStreams.addSource(commitSource);
