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

// pull related commits from other repos into the activity stream
var externalCommitSource = {
	name: function()
	{
		return("externalCommitSource");
	},

	module: 'scrum',

	dagsUsed: function()
	{
		return( [sg.dagnum.WORK_ITEMS ] );
	},

	getActivity: function(request, max, userid)
	{
		var results = [];
		var idToDesc = {};
		var descs = sg.list_descriptors();
		for ( var desc in descs )
		{
			var d = descs[desc];
			idToDesc[d.repo_id] = desc;
		}

		var repo = request.repo;
		var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);

		var recs = db.query('vc_changeset', ['commit','item','last_timestamp','history'], null, 'last_timestamp #DESC', max * 4);

		for ( var i = recs.length - 1; i >= 0; --i )
		{
			var matches = recs[i].commit.match(/^(.+):(.+)$/);

			if (! matches)
				continue;

			var repoid = matches[1];
			var csid = matches[2];

			desc = idToDesc[repoid];

			if (! desc)
				continue;

			if (repoid == repo.repo_id)
				continue;

			var audit = vv.getFirstAudit(recs[i]);

			var ext = null;

			var result = {
				action: "Associated Commit in " + desc,
				who: audit.userid,
				when: audit.timestamp,
				what: "",
				link: "/workitem.html?recid=" + recs[i].item
			};

			try
			{
				ext = sg.open_repo(desc);

				var cs = ext.get_changeset_description(csid);

				if (cs && cs.comments && (cs.comments.length > 0))
					result.what = cs.comments[0].text;

				var branches = ext.lookup_branch_names( {"changesets": [csid] });

				var bns = [];
				for (var bn in branches[csid])
					bns.push(bn);

				if (bns.length > 0)
					result.what += " (" + bns.join(', ') + ")";
			}
			catch (e)
			{
				// happens if one repo has been pushed/pulled before the other
				continue;
			}
			finally
			{
				if (ext)
					ext.close();
			}

			var rec = db.get_record('item', recs[i].item, ['id']);
			result.action = rec.id + " - " + result.action;

			results.push(result);
		}
		return(results);
	}

};


function bugIsRelevant(bug, userid)
{
	if (! userid)
		return(true);

	return( (bug.reporter == userid) ||
			(bug.assignee == userid) ||
			(bug.verifier == userid)
		  );
}



var bugSource = {
	name: function()
	{
		return("bugSource");
	},

	module: 'scrum',

	dagsUsed: function()
	{
		return( [sg.dagnum.WORK_ITEMS ]);
	},

	bugDiffs: function(lastbug, thisbug)
	{
		var changes = {};

		var v = function(val)
		{
			if (typeof(val) === 'undefined')
				return('');
			else
				return(val);
		};

		for ( var fn in lastbug )
			if (lastbug[fn] != v(thisbug[fn]))
			{
				changes[fn] = {
					then: lastbug[fn],
					now:  v(thisbug[fn])
				};
			}

		for ( fn in thisbug )
			if ((v(lastbug[fn]) != thisbug[fn]) && (! changes[fn]))
			{
				changes[fn] = {
					then: v(lastbug[fn]),
					now: thisbug[fn]
				};
			}

		delete changes.listorder;
		delete changes.id;
		delete changes.estimate;
		delete changes.generation;
		delete changes.csid;
		delete changes.timestamp;
		delete changes.userid;
		delete changes.username;

		return(changes);
	},

	describe: function(activity, diffs, bug, repo)
	{
		var lines = [];
		var action = false;

		if (diffs.status)
		{
			if (diffs.status.now == 'open')
			{
				action = "Reopened " + bug.id;
			}
			else if (diffs.status.now == 'fixed')
			{
				action = "Fixed " + bug.id;
			}
			else if (diffs.status.now == 'verified')
			{
				action = "Verified " + bug.id;
			}
			else
			{
				var stat = "Status: " + (diffs.status.now || '(unassigned)');

				if (diffs.status.then)
					stat += " (was " + diffs.status.then + ")";
				stat += ".";

				lines.push(stat);
			}

			activity._acttype = 'bugstatus';
		}

		if (diffs.assignee && diffs.assignee.now)
		{
			var username = vv.lookupUser(diffs.assignee.now, repo);

			if (username && ! action)
				action = "Assigned " + bug.id + " to " + username;
			else if (username)
				lines.push("Assigned to " + username + ".");
		}
		else if (diffs.assignee)
		{
			if (! action)
				action = "Unassigned " + bug.id;
			lines.push("Assigned to no one.");
		}


		if (diffs.verifier && diffs.verifier.now)
		{
			var username = vv.lookupUser(diffs.verifier.now, repo);

			if (username)
				lines.push("verifier is now " + username + ".");
		}
		else if (diffs.verifier)
			lines.push("verifier is now unassigned.");

		if (diffs.priority && diffs.priority.now)
		{
			var change = 'Priority is now ' + diffs.priority.now;

			if (diffs.priority.then)
				change += " (was " + diffs.priority.then + ")";

			change += ".";

			lines.push(change);
		}
		else if (diffs.priority)
			lines.push("Priority is now empty.");

		var others = [ 'title', 'description', 'milestone' ];

		for ( var i in others )
		{
			var fn = others[i];

			if (diffs[fn])
				if (diffs[fn].now)
					lines.push(fn + " is now '" + diffs[fn].now + "'.");
				else if (diffs[fn].then)
				{
					if (fn == 'milestone')
						lines.push(fn + " is now 'Product Backlog'");
					else
						lines.push(fn + " is now empty.");
				}
		}

		if (action)
		{
			activity.action = action;
			if (! activity._acttype)
				activity._acttype = 'bugmod';
		}

		if (lines.length > 0)
		{
			if (! activity._acttype)
				activity._acttype = 'bugmod';

			activity.what = lines.join("\n");
		}
	},

	listsize: function(list)
	{
		var size = 0;

		for ( var fn in list )
			if (list.hasOwnProperty(fn))
				++size;

		return(size);
	},

	getActivity: function(request, max, userid)
	{
		var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

		var where = null;

		if (userid)
		{
			where = vv.where({'reporter': userid});
			where = vv._addToWhere(where, 'assignee', userid, '||');
			where = vv._addToWhere(where, 'verifier', userid, '||');
		}

		var recentMods = db.query__recent('item', ['*'],
										  where,
										  max);

		return this.buildResults(recentMods, request.repo, db, max);
	},

	buildResults: function(recentMods, repo, db, max)
	{
		var recentRecords = {};

		for ( var i = 0; i < recentMods.length; ++i )
			recentRecords[recentMods[i].recid] = 1;

		var results = [];

		for ( var recid in recentRecords )
		{
			var lastbug = null;
			var skipFirst = false;

			var bugHistory = db.query__recent('item', ['*'], vv.where( { 'recid' : recid } ), max + 1);

			if (bugHistory.length > max)
				skipFirst = true;

			var first = true;

			var bugEnds = bugHistory.length - 1;

			for ( var j = bugEnds; j >= 0; --j )
			{
				var thisbug = bugHistory[j];
				var csid = thisbug.csid;

				var record = {
					what: thisbug.title,
					title: thisbug.title,
					who: thisbug.userid,
					when: thisbug.timestamp,
					_csid: csid,
					_bugid: recid
				};

				if (thisbug.milestone)
					thisbug.milestone = vv.lookupMilestone(thisbug.milestone, repo) || '';

				if (first)
				{
					record.action = "Created " + thisbug.id;
					record.description = thisbug.description || false;
					record._acttype = 'bugmod';
				}
				else
					record.action = "Modified " + thisbug.id;

				if (lastbug)
				{
					var diffs = this.bugDiffs(lastbug, thisbug);

					lastbug = thisbug;

					if (this.listsize(diffs) == 0)
					{
						first = false;
						continue;
					}

					this.describe(record, diffs, thisbug, repo);
				}
				else
					lastbug = thisbug;

				record.link = '/workitem.html?recid=' + recid;

				if (! (first && skipFirst))
					results.push(record);

				first = false;
			}
		}

		return(results);
	}
};


var relationSource = {
	name: function()
	{
		return("relationSource");
	},

	module: 'scrum',

	dagsUsed: function()
	{
		return( [sg.dagnum.WORK_ITEMS, sg.dagnum.VC_COMMENTS, sg.dagnum.VERSION_CONTROL, sg.dagnum.USERS] );
	},

	getActivity: function(request, max, userid)
	{
		var bugsNeeded = {};

		var w = null;

		if (userid)
		{
			w = '';

			w = vv._addToWhere(w, 'sourceassignee', userid, '||');
			w = vv._addToWhere(w, 'sourcereporter', userid, '||');
			w = vv._addToWhere(w, 'sourceverifier', userid, '||');
			w = vv._addToWhere(w, 'targetassignee', userid, '||');
			w = vv._addToWhere(w, 'targetreporter', userid, '||');
			w = vv._addToWhere(w, 'targetverifier', userid, '||');
		}

		var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

		var relations = db.query('relation',
					  [
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourcerecid', 'field': 'recid' },
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourceid', 'field': 'id' },
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourcetitle', 'field': 'title' },
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourceassignee', 'field': 'assignee' },
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourcereporter', 'field': 'reporter' },
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourceverifier', 'field': 'verifier' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targetrecid', 'field': 'recid' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targetid', 'field': 'id' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targettitle', 'field': 'title' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targetassignee', 'field': 'assignee' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targetreporter', 'field': 'reporter' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targetverifier', 'field': 'verifier' },
						  'relationship', 'history'
					  ],
					  w, "when #DESC", max || 0
					 );
		var results = [];

		for ( var i = 0; i < relations.length;  ++i )
		{
			var rel = relations[i];
			var rec = {};

			var relationship = 'related to';

			if (rel.relationship == 'blocks')
				relationship = "blocking";
			else if (rel.relationship == 'duplicateof')
				relationship = "a duplicate of";

			rec.what = relationship + " " + rel.targetid + " (" + rel.targettitle + ")";
			rec.action = rel.sourceid + " marked as";
			rec.link = "/workitem.html?recid=" + rel.sourcerecid;

			var audit = vv.getFirstAudit(rel);
			rec.when = audit.timestamp;
			rec.who = audit.userid;

			var hist = vv.getFirstHistory(rel);
			rec._csid = hist.csid;
			rec._sourceid = rel.sourceid;
			rec._bugid = rel.sourceid;

			results.push(rec);
		}

		results.sort(
			function(a,b)
			{
				var res = a._csid.localeCompare(b._csid);

				if (res == 0)
					res = a._sourceid.localeCompare(b._sourceid);

				return(res);
			}
		);

		for ( i = results.length - 2; i >= 0; --i )
		{
			var j = i + 1;

			if ((results[i]._csid == results[j]._csid) && (results[i]._sourceid == results[j]._sourceid))
			{
				results[i].what += "; " + results[j].what;
				results.splice(j, 1);
			}
		}

		return(results);
	}
};

var commentSource = {
	name: function()
	{
		return("commentSource");
	},

	module: 'scrum',

	dagsUsed: function()
	{
		return( [sg.dagnum.WORK_ITEMS, sg.dagnum.USERS] );
	},

	getActivity: function(request, max, userid)
	{
		var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

		var records = db.query("comment", ["*",'history'], null, "when #DESC", max);
		var results = [];

		for ( i in records )
		{
			var comment = records[i];

			var bug  = db.get_record('item', comment.item, ["*"]);

			if (userid)
				if ((comment.who != userid) && ! bugIsRelevant(bug, userid))
					continue;

			comment.what = comment.text;
			comment.link = "/workitem.html?recid=" + bug.recid + "#" + comment.recid;
			comment.title = bug.title;

			comment.action = "Commented on " + bug.id;
			comment._acttype = 'bugcomment';

			var hist = vv.getFirstHistory(comment);
			comment._csid = hist.csid;
			var audit = vv.getFirstAudit(comment);
			comment.when = audit.timestamp;
			comment._bugid = comment.item;

			delete comment.text;
			delete comment.item;
			delete comment.recid;
			delete comment.history;

			results.push(comment);
		}

		return(results);
	}
};

var attachmentSource = {
	name: function()
	{
		return("attachmentSource");
	},

	module: 'scrum',

	dagsUsed: function()
	{
		return( [sg.dagnum.WORK_ITEMS, sg.dagnum.USERS] );
	},

	getActivity: function(request, max, userid)
	{
		var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

		var records = db.query("attachment", ["recid",'last_timestamp','filename','item','history'], null, "last_timestamp #DESC", max);
		var results = [];

		for ( var i = 0; i < records.length; ++i )
		{
			var attachment = records[i];
			var audit = vv.getFirstAudit(attachment);
			var who = audit.userid;

			var bug  = db.get_record('item', attachment.item, ["*"]);

			if (userid)
				if ((who != userid) && ! bugIsRelevant(bug, userid))
					continue;

			attachment.what = attachment.filename;
			attachment.link = "/workitem.html?recid=" + bug.recid;
			attachment.title = bug.title;
			attachment.who = who;
			attachment.when = audit.timestamp;

			attachment.action = "Attachment added to " + bug.id;

			var hist = vv.getFirstHistory(attachment);
			attachment._csid = hist.csid;
			attachment._bugid = bug.recid;

			delete attachment.history;
			delete attachment.contenttype;
			delete attachment.data;
			delete attachment.filename;
			delete attachment.item;
			delete attachment.recid			;

			results.push(attachment);
		}

		return(results);
	}
};

var stampSource = {
	name: function()
	{
		return "stampSource";
	},

	module: 'scrum',

	dagsUsed: function()
	{
		return( [sg.dagnum.WORK_ITEMS, sg.dagnum.USERS] );
	},

	getActivity: function(request, max, userid)
	{
		if (!userid)
			return [];

		var results = [];
		var udb = new zingdb(request.repo, sg.dagnum.USERS);

		var user = udb.get_record("user", userid);

		var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

		// Get changes in bugs that have been tagged with this users name
		var all_user_stamps = db.query("stamp", ["*"], vv.where( { name: user.name }));
		var user_item_recids = all_user_stamps.map(function(v) { return v.item; });
		if (user_item_recids.length > 0)
		{
			var recentMods = db.query__recent("item", ["*"], vv.where( { recid: user_item_recids }), max);
			results = bugSource.buildResults(recentMods, request.repo, db, max);
		}

		// Get the most recent stamps created for this user
		var recent_user_stamps = db.query__recent("stamp", ["*"], vv.where( { name: user.name }), max);

		var recently_tagged_item_recids = [];
		var item_to_stamp_date = [];
		var item_to_stamp_user = {};
		for (var i = 0; i < recent_user_stamps.length; i++)
		{
			var stamp = recent_user_stamps[i];
			recently_tagged_item_recids.push(stamp.item);
			item_to_stamp_date[stamp.item] = stamp.timestamp;
			item_to_stamp_user[stamp.item] = stamp.userid;
		}

		if (recently_tagged_item_recids.length > 0)
		{
			var items = db.query("item", ["*"], vv.where( {recid: recently_tagged_item_recids }));

			for (i = 0; i < items.length; i++)
			{
				var item = items[i];
				var record = {
					what: "Stamp " + user.name + " added",
					action: item.id + " stamped",
					who: item_to_stamp_user[item.recid] || userid,
					when: item_to_stamp_date[item.recid],
					link: '/workitem.html?recid=' + item.recid
				};

				results.push(record);
			}
		}

		return results;
	}
};

activityStreams.addSource(commitSource);

activityStreams.addSource(commentSource);
activityStreams.addSource(bugSource);
activityStreams.addSource(stampSource);
activityStreams.addSource(relationSource);
activityStreams.addSource(attachmentSource);
activityStreams.addSource(externalCommitSource);

