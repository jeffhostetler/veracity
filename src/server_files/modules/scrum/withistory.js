/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,	
}

WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


registerRoutes({
	"/repos/<repoName>/workitem/<recid>/history.json": {
		"GET": {
			onDispatch: function (request) {

				var cc = new textCache(request, [sg.dagnum.WORK_ITEMS]);
				return (cc.get_response
				(
					function(request)
					{
						var hist = new vvWitHistory(request, request.recid);

						var results = hist.getHistory();

						if (! results)
							throw notFoundResponse();

						return results;
					}, 
					this, request)
				);
			}
		}
	}
});



function vvWitHistory(request, bugid)
{
	this.bugid = bugid;
	this.repoName = request.repoName;
	this.repo = request.repo;

	this.fnTrans = 	{
		"title": "Name",
		"assignee": "Assigned To",
		"estimate": "Estimate"
	};

	return(this);
}


vvWitHistory.prototype.getHistory = function()
{
	if (!this.repo)
		return(false);

	var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);
	var results = [];

	var rec = db.get_record('item', this.bugid, ['*', 'history']);

	if (! rec)
		return(false);

	for ( var i = rec.history.length - 1; i >= 0; --i )
	{
		var audit = rec.history[i].audits[0];
		rec._when = audit.timestamp;
	}
	
	rec.history.sort(
		function(a,b)
		{
			return( b._when - a._when );
		}
	);

	var first = true;
	var lastbug = {};

	for ( i = rec.history.length - 1; i >= 0; --i )
	{
		var rrec = {};
			
		var hid = rec.history[i].hidrec;
		var audit = rec.history[i].audits[0];
		var csid = rec.history[i].csid;
			
		rrec.csid = csid;
		rrec.when = audit.timestamp;
		rrec.who = vv.lookupUser(audit.userid, this.repo);

		if (first)
			rrec.action = "Created";
		else
			rrec.action = "Modified";

		first = false;

		var thisbug = this.repo.fetch_json(hid);
		delete thisbug.comments;

		if (thisbug.assignee)
			thisbug.assignee = vv.lookupUser(thisbug.assignee, this.repo);
		if (thisbug.reporter)
			thisbug.reporter = vv.lookupUser(thisbug.reporter, this.repo);
		if (thisbug.verifier)
			thisbug.verifier = vv.lookupUser(thisbug.verifier, this.repo);
		if (thisbug.milestone)
			thisbug.milestone = vv.lookupMilestone(thisbug.milestone, this.repo);

		var diffs = this.bugDiffs(lastbug, thisbug);
		lastbug = thisbug;

		var vvh = this;
		var any = false;

		vv.each
		(
			diffs,
			function(fn, diff)
			{
				var recfn = vvh.fnToTitle(fn);
				rrec[recfn] = diff['now'];
				any = true;
			}
		);

		results.push(rrec);
	}

	var cmts = db.query('comment', ['*','history'], vv.where( { 'item': this.bugid } ));
	var repo = this.repo;

	vv.each
	(
		cmts,
		function(i, cmt)
		{
			var crec = {};
				
			crec.who = vv.lookupUser(cmt.who, repo);
			crec.when = cmt.when;
			crec.comment = cmt.text;
			crec.action = "Comment";
			
			var hist = vv.getFirstHistory(cmt);
			if (hist)
				crec.csid = hist.csid;

			results.push(crec);
		}
	);

	var attachments = db.query('attachment', ['*','history'], vv.where( { 'item': this.bugid } ));

	vv.each
	(
		attachments,
		function(i, att)
		{
			var ahist = vv.getFirstAudit(att);
			if (! ahist)
				return;

			var crec = {
				who: vv.lookupUser(ahist.userid, repo),
				when: ahist.timestamp,
				action: "File Attached",
				attachment: att.filename
			};
				
			results.push(crec);
		}
	);
		
	var work = db.query("work", ["*", "history"], vv.where( { "item": this.bugid }));
		
	vv.each(work, function(i,rec) {
		var ahist = vv.getFirstAudit(rec);
		if (! ahist)
			return;
			
		var wrec = {
			who: vv.lookupUser(ahist.userid, repo),
			when: rec.when,
			action: "Logged Work",
			amount: rec.amount
		}
			
		results.push(wrec);
	});

	var stamps = db.query("stamp", ["*", "history"], vv.where( { "item": this.bugid }));
	
	vv.each(stamps, function(i, rec) {
		var ahist = vv.getFirstAudit(rec);
		if (! ahist)
			return;

		var hist = vv.getFirstHistory(rec);
		
		var srec = {
			who: vv.lookupUser(ahist.userid, repo),
			when: ahist.timestamp,
			action: "Stamp Added",
			stamp: rec.name,
			csid: hist.csid
		};
		
		results.push(srec);
	});


	this.addRelations(results, this.bugid);
	
	results.sort(
		function(a,b)
		{
			return( b.when - a.when );
		}
	);

	for ( i = 0; i < results.length; ++i )
	{
		vv.each(["Related To", "Blocks", "Depends On", "Duplicate Of", "Duplicated By", "commit"],
				function(j, fn)
				{
					if (results[i][fn])
						results[i][fn] = [ results[i][fn] ];
				}
			   );
	}
	
	i = 0;

	while (i < (results.length - 1))
	{
		if (! results[i].csid)
			vv.log("no csid", results[i]);
		if (results[i].csid && (results[i].csid == results[i + 1].csid))
		{
			this.mergeResults(results, i, i + 1);
		}
		else
			++i;
	}

	return( results );
};

vvWitHistory.prototype.mergeResults = function(results, ai, bi)
{
	var commentPref = [
		"Created",
		"Modified",
		"Stamp Added",
		"File Attached",
		"Relationship added",
		"Associated Commit",
		"Logged Work",
		"Comment"
	];

	var a = results[ai];
	var b = results[bi];

	// pick the first matching action we see
	for ( var i = 0; i < commentPref.length; ++i )
		if ((a.action == commentPref[i]) || (b.action == commentPref[i]))
		{
			a.action = commentPref[i];
			break;
		}

	for ( fn in b )
	{
		if ( (["Related To", "Blocks", "Depends On", "Duplicate Of", "Duplicated By", "commit"].indexOf(fn) >= 0) && a[fn]) 
			a[fn] = a[fn].concat(b[fn]);
		else if (a[fn] === undefined)
			a[fn] = b[fn];
	}

	results.splice(bi, 1);
};

vvWitHistory.prototype.addRelations = function(results, bugid)
{
	var bugsNeeded = {};

	var w = '';

	w = vv._addToWhere(w, 'source', bugid, '||');
	w = vv._addToWhere(w, 'target', bugid, '||');

	var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

	var relations = db.query('relation',
							 [
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourcerecid', 'field': 'recid' },
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourceid', 'field': 'id' },
						  { 'type': 'from_me', 'ref': 'source', 'alias': 'sourcetitle', 'field': 'title' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targetrecid', 'field': 'recid' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targetid', 'field': 'id' },
						  { 'type': 'from_me', 'ref': 'target', 'alias': 'targettitle', 'field': 'title' },
						  'relationship', 'history'
					  ],
					  w, "when #DESC"
					 );

	for ( var i = 0; i < relations.length;  ++i )
	{
		var rel = relations[i];
		var rec = {};
		var fromMe = rel.sourcerecid == bugid;

		var relationship = 'Related To';

		if ((rel.relationship == 'blocks') && fromMe)
			relationship = "Blocks";
		else if (rel.relationship == 'blocks')
			relationship = 'Depends On';
		else if ((rel.relationship == 'duplicateof') && fromMe)
			relationship = "Duplicate Of";
		else if (rel.relationship == 'duplicateof')
			relationship = "Duplicated By";

		var otherId = fromMe ? rel.targetid : rel.sourceid;
		var otherTitle = fromMe ? rel.targettitle : rel.sourcetitle;

		rec.action = "Relationship added";
		rec[relationship] = otherId + " (" + otherTitle + ")";

		
		var audit = vv.getFirstAudit(rel);
		rec.when = audit.timestamp;
		rec.who = vv.lookupUser(audit.userid, this.repo);

		var hist = vv.getFirstHistory(rel);
		rec.csid = hist.csid;

		results.push(rec);
	}

	var idToDescriptor = {};
	var descs = sg.list_descriptors();

	for ( var dn in descs )
	{
		var descriptor = descs[dn];
		var id = descriptor.repo_id;
		if (! idToDescriptor[id])
			idToDescriptor[id] = dn;
	}

	relations = db.query('vc_changeset', ['*','history'], vv.where( { 'item': bugid }), 'when #DESC');

	for ( i = 0; i < relations.length;  ++i )
	{
		var rel = relations[i];
		var rec = {
			action: "Associated Commit"
		};

		var audit = vv.getFirstAudit(rel);
		rec.when = audit.timestamp;
		rec.who = vv.lookupUser(audit.userid, this.repo);
		rec.commit = null;

		var hist = vv.getFirstHistory(rel);
		rec.csid = hist.csid;

		var commit = rel.commit;
		var orepo = null;

		try
		{
			var lookupRepo = this.repo;
			var csid = commit;

			var matches = commit.match(/^(.+):(.+)$/);
			var repotitle = '';

			if (matches)
			{
				var lookuprepid = matches[1];
				csid = matches[2];

				var desc = idToDescriptor[lookuprepid];

				if (desc)
				{
					lookupRepo = sg.open_repo(desc);
					orepo = lookupRepo;
					repotitle = '(' + desc + ') ';
				}
				else
					lookupRepo = null;
			}

			if (lookupRepo)
			{
				var desc = lookupRepo.get_changeset_description(csid);

				if (desc)
				{
					rec.commit = repotitle + desc.revno + ":" + csid;

					if (desc.comments && (desc.comments.length > 0))
						rec.comment = desc.comments[0].text;
				}
				else
					rec.commit = null;
			}
			else
				rec.commit = null;
		}
		catch(e)
		{
			rec.comment = null;
			vv.log("Error retrieving info on commit", commit, e.toString());
		}
		finally
		{
			if (orepo)
				orepo.close();
		}

		if (rec.commit !== null)
			results.push(rec);
	}


	return(results);
};


vvWitHistory.prototype.bugDiffs = function(lastbug, thisbug)
{
	var changes = {};

	for ( var fn in lastbug )
		if (lastbug[fn] || thisbug[fn])
			if (lastbug[fn] != thisbug[fn])
			{
				changes[fn] = {
					then: lastbug[fn],
					now:  thisbug[fn] || ''
				};
			}

	for ( fn in thisbug )
		if ((lastbug[fn] != thisbug[fn]) && (! changes[fn]))
		{
			if (lastbug[fn] || thisbug[fn])
				changes[fn] = {
					then: '',
					now: thisbug[fn]
				};
		}

	delete changes.rectype;

	return(changes);
};




vvWitHistory.prototype.fnToTitle = function(fn)
{
	if (this.fnTrans[fn])
		return(this.fnTrans[fn]);
	
	return(fn);
};


