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


/*
 * witRequest does most of the heavy lifting, fetching/updating links for a specified rectype
 * 
 * textCache.js handles caching
 * 
 * the route table consists largely of wrapper calls to those classes
 * 
 * currently implemented:
 *   GET /repos/<repoName>/workitems.json
 *   retrieve all 'item' records as a JSON array
 *   optional parameters: _includelinked, _sort, _desc, _fields, _skip, _returnmax, q
 * 
 *   POST /repos/<repoName>/workitems.json
 *   add a new work item, from JSON in the request body
 *   returns the new item's recid as plain text
 * 
 *   GET /repos/<repoName>/workitem/<recid>.json
 *   return JSON representation of the work item matching <recid>
 *   optional parameters: _includelinked, _fields
 *   if _includelinked is specific, we always return an array, even if no linked records are found
 *   otherwise we return the single record
 * 
 *   PUT /repos/<repoName>/workitem/<recid>.json
 *   update work item <recid>
 * 
 *   DELETE /repos/<repoName>/workitem/<recid>.json
 *   delete work item <recid>
 * 
 *   PUT /repos/<repoName>/milestone/<recid>/<linkname>
 *   PUT /repos/<repoName>/workitem/<recid>/<linkname>
 *   if the request body contains a single link record, set a single link to that record, or add it to a multiple-link list
 *   if a "linklist" array is passed, replace the entire link list with this array
 * 
 *   POST /repos/<repoName>/workitem/<recid>/attachments
 *   attach a posted file to work item <recid>
 *   return the attachment's recid as plain text
 *   
 *   GET /repos/<repoName>/milestone/<recid>/<linkname>.json
 *   GET /repos/<repoName>/workitem/<recid>/<linkname>.json
 *   get the list of records linked as recid.linkname
 * 
 *   POST /repos/<repoName>/milestones
 *   add a new milestone, from JSON in the request body
 *   returns the new milestones' recid as plain text
 * 
 *   GET /repos/<repoName>/milestones.json
 *   retrieve the list of milestones
 *   optional parameters: _includelinked, _sort, _desc, _fields, _skip, _returnmax, q, _unreleased
 * 
 *   GET /repos/<repoName>/milestone/<recid>.json
 *   return JSON representation of the milestone matching <recid>
 *   optional parameters: _includelinked, _fields
 *   if _includelinked is specific, we always return an array, even if no linked records are found
 *   otherwise we return the single record
 * 
 *  --
 * 
 *   _includelinked=1
 *     follow any link fields found in the requested record(s)
 *     include those records in the results, with a _secondary field added to each
 *   _sort=field
 *     sort the results on the specified field
 *   _desc=1
 *     sort the results in descending order by the _sort field
 *   _returnmax=N
 *     return no more than N (primary) records
 *   _skip=N
 *     skip the first N results
 *   q=searchtext
 *     return only (primary) records containing 'searchtext' in one of their fields
 *   _unreleased=1
 *     return only unreleased milestones
 * 
 */
 

function witRequest(request, linkfields)
{
	this.params = {
		_fields: null,
		recid: null,
		_includelinked: false,
		_sort: null,
		_ids: null,
		q: null,
		_desc: null,
		rectype: null,
		_returnmax: null,
		_skip: null,
		_unreleased: false,
		_csid: null,
		_allVersions: false,
		_getlinks: false,
		_returnrecord: false,
		userid: null,
		term: null,
		prefix: null
	};
	this.linkfields = linkfields || {};
	this.fields = ["*"];
	this.request = request;		
	this.query = {};
	this.searchstr = "";

	var q = request.queryString || "";		

	var sections = q.split('&');

	for ( var i = 0; i < sections.length; ++i )
	{
		var name = "", value = "";

		var parts = sections[i].split('=');

		name = decodeURIComponent(parts[0]);	
	
		if (parts.length > 1)
			value = decodeURIComponent(parts[1]);

		if (this.params[name] !== undefined)
			this.params[name] = value;
		else if (name && (name != '_'))
			this.query[name] = value;
	}

	if (this.params._includelinked)
		this.params._getlinks = true;

	if (this.params._fields)
		this.fields = this.params._fields.split(',');

	var bang = this.fields.indexOf("!");

	if (bang >= 0)
	{
		this.fields.splice(bang, 1);
		this.params._getlinks = true;
	}
}
	
witRequest.prototype.checkSide = function(linkdetails, otherside, rectype, fromMe) 
{
	if (! linkdetails)
		return;
	
	var us = false;
	
	for ( rt in linkdetails.link_rectypes )
		if (linkdetails.link_rectypes[rt] == rectype)
	{
		us = true;
		break;
	}
	
	if (us)
	{
		var linkname = 	linkdetails.name;
		var multiple = ! linkdetails.singular;
		
		this.linkfields[linkname] = { isMultiple: multiple, isFromMe: fromMe, otherType: otherside.link_rectypes[0] };
	}
};
	
witRequest.prototype.findLinkFields = function(rectype, db) 
{
	for ( var ff in this.linkfields )
	{
		return;
	}

	if (! db)
	{
		db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
	}

	var template = db.get_template();
		
	if (! template.directed_linktypes)
	{
		return;
	}
		
	for ( var lt in template.directed_linktypes )
	{
		var details = template.directed_linktypes[lt];
			
		this.checkSide(details.from, details.to, rectype);
		this.checkSide(details.to, details.from, rectype);
	}
};

witRequest.prototype.fetchLinkRecords = function(results, rec, linkfields, db, includeRecords)
{
	if (! db)
	{
		db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
	}

	var recordsSeen = {};
	var recid = rec.recid;

	for ( var j = 0; j < results.length; ++j )
	{
		var oldrec = results[j];
		recordsSeen[oldrec.recid] = true;
	}
		
	for ( var fn in linkfields )
	{
		var link = linkfields[fn];

		var multi = link.isMultiple;
			
		if (link.isFromMe)
		{
			if (rec[fn])
			{
				var links;
				
				if (multi)
					links = rec[fn];
				else
					links = [ rec[fn] ];
				
				for ( var i in links )
				{
					var ln = links[i];

					if (ln)
						if (! recordsSeen[ln])
						{
							var linkrec = db.get_record(link.otherType, ln);
							linkrec._secondary = true;
							linkrec.rectype = link.otherType;
						
							if (includeRecords)
							{
								linkrec._secondary = true;
								results.push(linkrec);
							}

							recordsSeen[ln] = true;
						}
				}
			}
		}
		else
		{
			var params = {};

			var targetField = link.otherLinkName || link.linkTarget;

			vv.extend(params, link.values);
				
			params[targetField] = recid;
			var q = vv.where( params );

			var rt = link.linkType || link.otherType;

			var fields = this.fields || ['*'];

			var recs = db.query(rt, fields, q);

			for ( var i = 0; i < recs.length; ++i)
			{
				recs[i]._secondary = true;
				recs[i].rectype = rt;
			}

			var recidField = 'recid';
			if (link.linkType)
				recidField = link.linkSource;

			if (multi)
				rec[fn] = [];

			for ( i = 0; i < recs.length; ++i )
			{
				var rid = recs[i][recidField];
				if (! recordsSeen[rid])
				{
					if (includeRecords)
					{
						recs[i]._secondary = true;
						results.push(recs[i]);
					}

					recordsSeen[rid] = true;
				}

				if (multi)
					rec[fn].push(rid);
				else
					rec[fn] = rid;
			}
		}
	}
};


witRequest.prototype.addRecord = function(rectype, newrec)
{
	if ((newrec.rectype) && (newrec.rectype != rectype))
		return( badRequestResponse() );

	delete newrec.rectype;

	var request = this.request;

	var db;
	var ztx = false;

	try
	{
		db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
		ztx = db.begin_tx(null, request.vvuser);

		var zrec = ztx.new_record(rectype);

		delete newrec._csid;

		for ( var f in newrec )
			zrec[f] = newrec[f];

		var recid = zrec.recid;

		vv.txcommit(ztx);
						
		ztx = null;

		db.merge(request.vvuser);

		return( textResponse( recid ));
	}
	catch (e)
	{
		if (ztx)
			ztx.abort();

		vv.log(e.toString());
		
		return( badRequestResponse() );
	}

	return( badRequestResponse() );
};

witRequest.prototype.getComments = function(recid)
{
	var request = this.request;

	var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

	var q = vv.where(
		{
			'item': recid
		}
	);

	var recs = db.query('comment', ['*'], q);

	return( jsonResponse( recs ) );
};

witRequest.prototype.addComment = function(cmtData)
{
	var request = this.request;

	var db;
	var ztx = null;

	try
	{
		db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

		var rec = db.get_record('item', request.recid);
		
		if (! rec)
			return( notFoundResponse() );

		ztx = db.begin_tx(null, request.vvuser);

		var zrec = ztx.new_record('comment');
		var cmtrecid = zrec.recid;
		zrec.text = cmtData.text;
		zrec.item = rec.recid;
		ztx.commit();
		ztx = null;

		return( textResponse( cmtrecid ) );
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.addWork = function(wdata)
{
	var request = this.request;

	var db;
	var ztx = null;

	try
	{
		db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

		var rec = db.get_record('item', request.recid);
		
		if (! rec)
			return( notFoundResponse() );

		ztx = db.begin_tx(null, request.vvuser);
		
		if (wdata.amount == null && wdata.estimate == null)
			return badRequestResponse();
			
		// This seems really broken, but its necesary
		// Sometimes the work form will only be changing the estimate
		if (wdata.amount)
		{
			var zrec = ztx.new_record('work');
			var wrecid = zrec.recid;
			zrec.when = wdata.when;
			zrec.item = request.recid;
			zrec.amount = wdata.amount;
		}
		
		if (wdata.estimate != null)
		{
			var item = ztx.open_record("item", request.recid);
			item.estimate = wdata.estimate; 
		}
		
		ztx.commit();
		ztx = null;
		
		if (wrecid)
			return( jsonResponse( wrecid ) );
		else
			return OK;
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.updateWork = function(wdata)
{
	var request = this.request;

	var db;
	var ztx = null;

	try
	{
		db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

		var rec = db.get_record('work', request.recid);
		
		if (! rec)
			return( notFoundResponse() );

		ztx = db.begin_tx(null, request.vvuser);

		var zrec = ztx.open_record('work', request.recid);
		zrec.when = wdata.when;
		zrec.amount = wdata.amount;
		
		if (wdata.estimate != undefined)
		{
			var item = ztx.open_record("item", request.recid);
			item.estimate = wdata.estimate; 
		}
		
		ztx.commit();
		ztx = null;

		return( jsonResponse( wrecid ) );
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.updateWorkitemWork = function(data)
{
    var request = this.request;

    var db;
    var ztx = null;

    try
    {
        db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
        ztx = db.begin_tx(null, request.vvuser);

        var item = null;

        if (!request.recid)
        {
            ztx.abort();
            ztx = false;
            return badRequestResponse();
        }

        var rec = ztx.open_record("item", request.recid);
        rec.estimate = data.estimate;

        if (data.work)
        {
            for (var i = 0; i < data.work.length; i++)
            {
                item = data.work[i];

                if (item.rectype != "work")
                    continue;

                if (item.recid)
                {
                    var wrec = db.get_record("work", item.recid);

                    if (!wrec)
                    {
                        ztx.abort();

                        ztx = false;
                        return (notFoundResponse(request, "Work record " + item.recid + " not found"));
                    }

                    if (item._delete)
                    {
                        ztx.delete_record("work", item.recid);
                    }
                    else
                    {
                        var zrec = ztx.open_record("work", item.recid);
                        if (!zrec.when)
                        {
                            zrec.when = new Date().getTime();
                        }
                        zrec.when = item.when;
                        zrec.amount = item.amount;
                        zrec.item = request.recid;                        
                    }
                }
                else
                {
                    if (item._delete)
                    {
                        continue;
                    }
                    var zrec = ztx.new_record("work");
                    if (!zrec.when)
                    {
                        zrec.when = new Date().getTime();
                    }
                    zrec.when = item.when;
                    zrec.amount = item.amount;
                    zrec.item = request.recid;                    
                }
            }
        }
       
        vv.txcommit(ztx);
        ztx = false;

        db.merge(request.vvuser);

        return (OK);
    }
    finally
    {
        if (ztx)
            ztx.abort();
    }
};

witRequest.prototype.batchUpdateItems = function(mods)
{
	var byCsid = {};

	for ( var i = 0; i < mods.length; ++i )
	{
		var rec = mods[i];
		var csid = rec._csid || '';
		byCsid[csid] = byCsid[csid] || [];

		byCsid[csid].push(rec);
	}

	var request = this.request;
	var repo = request.repo;
	var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
	var ztx = null;

	try
	{
		for ( var csid in byCsid )
		{
			ztx = db.begin_tx(csid || null, request.vvuser);
			var recs = byCsid[csid];

			for ( var i = 0; i < recs.length; ++i )
			{
				var newrec = recs[i];
				var recid = newrec.recid;
				delete newrec.rectype;
				delete newrec._csid;

				var zrec = ztx.open_record('item', recid);
				
				if (!zrec)
					throw "record not found: " + recid;

				for (var f in newrec)
					if (f != 'recid')
						zrec[f] = newrec[f];
			}

			var commit = vv.txcommit(ztx);
			ztx = false;

			db.merge(request.vvuser);
		}

	}
	finally
	{
		if (ztx)
			ztx.abort();
	}

	var results = [];

	for ( i = 0; i < mods.length; ++i )
	{
		var rec = db.get_record('item', mods[i].recid, ['*','history']);

		var h = vv.getLastHistory(rec);
		rec._csid = h.csid;
		delete rec.history;

		results.push(rec);
	}

	return(results);
};


witRequest.prototype.updateRecord = function(rectype, newrec)
{
    if ((newrec.rectype) && (newrec.rectype != rectype))
        return (badRequestResponse());

    delete newrec.rectype;
    delete newrec.recid;

    var request = this.request;

    var db;
    var ztx = false;

    try
    {
        var csid = null;

        if (newrec._csid)
        {
            csid = newrec._csid;
        }

        delete newrec._csid;

        db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
        ztx = db.begin_tx(csid, request.vvuser);

        var zrec = ztx.open_record(rectype, request.recid);

        if (!zrec)
            return (notFoundResponse());

        for (var f in newrec)
        {
            zrec[f] = newrec[f];
        }

        var commit = vv.txcommit(ztx);
        ztx = false;

        db.merge(request.vvuser);

        if (this.params._returnrecord)
        {
            var leaves = db.get_leaves();
			var newcsid = null;

            if (leaves.length == 1)
				newcsid = leaves[0];
			else
				newcsid = commit.csid;

            var bug = db.get_record('item', request.recid, ['*']);

            if (!bug)
                return (badRequestResponse());

            bug._csid = newcsid;

            return (jsonResponse(bug));
        }

        return (OK);
    }
    finally
    {
        if (ztx)
            ztx.abort();
    }
};

witRequest.prototype.deleteRecord = function(rectype)
{
	var ztx = false;

	var request = this.request;
	
	try
	{
		var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

		var oldrec = db.get_record(rectype, request.recid);

		if (! oldrec)
			return( notFoundResponse() );

		if ((oldrec.rectype) && (oldrec.rectype != rectype))
			return( badRequestResponse() );
		
		ztx = db.begin_tx(null, request.vvuser);

		ztx.delete_record(rectype, request.recid);

		vv.txcommit(ztx);
		ztx = false;

		db.merge(request.vvuser);

		return(OK);
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.fetchRecords = function (rectype)
{
    var zs = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
	var leaves = zs.get_leaves();

	if (leaves.length != 1)
		zs.merge(this.request.vvuser);

	leaves = zs.get_leaves();

	if (leaves.length != 1)
		return( preconditionFailedResponse(this.request, "Work items DAG has multiple heads") );

	var csid = leaves[0];

    var results = [];
    var records = [];

    var recid = this.request.recid || this.params.recid;
    rectype = rectype || this.params.rectype;

    this.findLinkFields(rectype, zs);

    if (recid)
    {
		var rec = null;

		if (this.params._allVersions)
		{
			rec = zs.get_record_all_versions(rectype, recid);
				
			if (!rec)
			{
				return (null);
			}

			return rec;
		}
		else
		{
			if (!this.params._csid)
			{
	            rec = zs.get_record(rectype, recid, this.fields);
			}
			else
			{
				// Remove links because this will not behave as expected
				this.fields= ["*"];
				
				rec = zs.get_record(rectype, recid, this.fields, this.params._csid);
			}
		}
			
        if (!rec)
        {
            return (null);
        }

		rec.rectype = rectype;

		records.push(rec);
	}
	else
	{
		var where = null;
		var sort = null;
		var skip = this.params._skip || 0;
		var limit = this.params._returnmax || 0;
		var anyCriteria = true;
		var criteria = {};
			
		if (this.params._sort)
			sort = this.params._sort;

        if (this.params._desc && sort)
            sort += " #DESC";

		for ( var fn in this.query )
		{
			var val = this.query[fn];
			var isLink = this.linkfields[fn];
			var isEmpty = (! val);

			var choices = [];

			if (isLink && isEmpty)
				choices.push(null);
			else if (isEmpty)
			{
				choices.push(null);
				choices.push('');
			}
			else
				choices.push(val);

			criteria[fn] = choices;
			anyCriteria = true;
		}

		if (this.params._unreleased)
		{
			criteria['releasedate'] = [ null ];
			anyCriteria = true;
		}

		if (anyCriteria)
			where = vv.where(criteria);
			
		skip = parseInt(skip);
		limit = parseInt(limit);
			
		records = zs.query(rectype, this.fields, where, sort, limit, skip);

		for ( var i = 0; i < records.length; ++i )
			records[i].rectype = rectype;
	}

    results = records;

    if (this.params.q)
    {
        for (var j = results.length - 1; j >= 0; --j)
        {
            var keep = false;
            var rec = results[j];

            for (var fname in rec)
            {
                if (typeof (rec[fname]) == 'string')
                {
                    if (rec[fname].indexOf(this.params.q) >= 0)
                    {
                        keep = true;
                        break;
                    }
                }
            }

            if (!keep)
                results.splice(j, 1);
        }
    }

	for ( var j = 0; j < results.length; ++j )
		results[j]._csid = csid;

    if (this.params._getlinks)
    {
        for ( j = 0; j < records.length; ++j )
        {
            var rec = records[j];

            this.fetchLinkRecords(results, rec, this.linkfields, zs, this.params._includelinked);
        }
    }

    return (results);
};

witRequest.prototype.searchStamps = function()
{
	var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
	var leaves = db.get_leaves();

	if (leaves.length != 1)
		db.merge(this.request.vvuser);

	leaves = db.get_leaves();

	if (leaves.length != 1)
		return( preconditionFailedResponse(this.request, "Work items DAG has multiple heads") );

	var csid = leaves[0];
	var results = [];
		
	// Query for stamp records
	var indistinctResults = [];
	var where = null;
	if (this.params.prefix)
		where = "name #MATCH " + vv._sqlEscape(this.params.prefix + '*');

	indistinctResults = db.query("stamp", ["name"], where);
		
	for (var i = 0; i < indistinctResults.length; i++)
	{
		var stamp = indistinctResults[i];
		if (results.indexOf(stamp.name) == -1)
			results.push(stamp.name);
	}
	
	indistinctResults = db.query("stamp", ["name"], vv.where( { name: this.params.prefix}));
	for (var i = 0; i < indistinctResults.length; i++)
	{
		var stamp = indistinctResults[i];
		if (results.indexOf(stamp.name) == -1)
			results.push(stamp.name);
	}
	
	var udb = new zingdb(this.request.repo, sg.dagnum.USERS);
	indistinctResults = [];
	indistinctResults = udb.query("user", ["name"], where);
		
	for (i = 0; i < indistinctResults.length; i++)
	{
		var user = indistinctResults[i];
		if (results.indexOf(user.name) == -1)
			results.push(user.name);
	}
	
	return results;
};

witRequest.prototype.search = function(fields) {
	var where = '';
	var term = this.params.term;

	var foundRecs = {};

	var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

	for ( var i = 0; i < fields.length; ++i )
	{
		var recs = db.query('item', ['*'], fields[i] + " #MATCH " + vv._sqlEscape(term + '*', true));

		for ( var j = 0; j < recs.length; ++j )
			foundRecs[ recs[j].recid ] = recs[j];
	}

	var results = [];

	for ( var recid in foundRecs)
		results.push(foundRecs[recid]);

	return(results);
};

witRequest.prototype.witDetails = function() {
	var where = '';
	var ids = this.params._ids.split(',');

	if (ids.length < 1)
		return( [] );

	var foundRecs = {};

	var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
	var w = vv.where( { 'id': ids } );

	var recs = db.query('item', ['*'], w);

	return(recs);
};

witRequest.prototype.addCommit = function() 
{
	var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
	var ztx = null;

	var recs = db.query('vc_changeset', ['recid'], 
						vv.where( { 'commit': this.request.csid, 'item': this.request.recid } )
					   );

	if (recs.length > 0)
		throw("association already exists");

	try
	{
		ztx = db.begin_tx(null, this.request.vvuser);
		var rec = ztx.new_record('vc_changeset');
		rec.item = this.request.recid;
		rec.commit = this.request.csid;
		var recid = rec.recid;
		vv.txcommit(ztx);
		ztx = null;
				   
		return( recid );
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.deleteCommit = function()
{
	var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
	var recs = db.query('vc_changeset', ['recid'], 
						vv.where( { 'commit': this.request.csid, 'item': this.request.recid } )
					   );

	var ztx = null;

	try
	{
		if (recs.length > 0)
			ztx = db.begin_tx(null, this.request.vvuser);

		for ( var i = 0; i < recs.length; ++i )
			ztx.delete_record('vc_changeset', recs[i].recid);

		vv.txcommit(ztx);
		ztx = null;
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.cleanRelList = function(rels, already) {
	for ( var i = rels.length - 1; i >= 0; --i )
	{
		var rel = rels[i];

		if (already[rel.recid])
		{
			rels.splice(i,1);
		}
		else
			already[rel.recid] = true;
	}

	return(rels);
};

witRequest.prototype.fullWorkItem = function ()
{
    var request = this.request;
	var noCache = false;

    if (!this.request.repo)
        return (null);

    var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

    var leaves = db.get_leaves();

    if (leaves.length != 1)
        return (preconditionFailedResponse("Work items DAG has multiple heads"));

    var csid = leaves[0];

    var bug = db.get_record('item', request.recid,
							['id', 'title', 'description', 'assignee', 'reporter', 'verifier', 'status', 'priority', 'milestone', 'estimate', 'history', 'recid', 'created_date',
								{
								    'type': 'to_me', 'rectype': 'comment', 'ref': 'item', 'fields': ['text', 'who', 'when', 'recid'], 'alias': 'comments'
								}
							]
							);
    if (!bug)
        return (null);
    bug.rectype = 'item';   // vForm wants this to match
    bug._csid = csid;

    var already = {};

    // todo: refactor all queries into two (source & blockers) then sort 
    var blockers = db.query('relation', [{ 'type': 'from_me', 'ref': 'source', 'alias': 'other', 'field': 'recid' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'id', field: 'id' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'status', field: 'status' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'title', field: 'title' },
											'recid'
										],
							vv.where({ 'target': request.recid, 'relationship': 'blocks' })
							);

    if (blockers)
        bug.blockers = this.cleanRelList(blockers, already);

    var blocking = db.query('relation', [{ 'type': 'from_me', 'ref': 'target', 'alias': 'other', 'field': 'recid' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'id', field: 'id' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'status', field: 'status' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'title', field: 'title' },
											'recid'
										],
							vv.where({ 'source': request.recid, 'relationship': 'blocks' })
							);

    if (blocking)
        bug.blocking = this.cleanRelList(blocking, already);

    var duping = db.query('relation', [{ 'type': 'from_me', 'ref': 'target', 'alias': 'other', 'field': 'recid' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'id', field: 'id' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'status', field: 'status' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'title', field: 'title' },
											'recid'
										],
							vv.where({ 'source': request.recid, 'relationship': 'duplicateof' })
							);

    if (duping)
        bug.duplicateof = this.cleanRelList(duping, already);

    var dups = db.query('relation', [{ 'type': 'from_me', 'ref': 'source', 'alias': 'other', 'field': 'recid' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'id', field: 'id' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'status', field: 'status' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'title', field: 'title' },
											'recid'
										],
							vv.where({ 'target': request.recid, 'relationship': 'duplicateof' })
							);

    if (dups)
        bug.duplicates = this.cleanRelList(dups, already);

    var relfrom = db.query('relation', [{ 'type': 'from_me', 'ref': 'target', 'alias': 'other', 'field': 'recid' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'id', field: 'id' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'status', field: 'status' },
											{ 'type': 'from_me', 'ref': 'target', alias: 'title', field: 'title' },
											'recid'
										],
							vv.where({ 'source': request.recid, 'relationship': 'related' })
							);
    if (relfrom && (relfrom.length > 0))
        bug.related = this.cleanRelList(relfrom, already);

    var attachments = db.query('attachment', ['*'], vv.where({ 'item': request.recid }));
    if (attachments && (attachments.length > 0))
        bug.attachments = attachments;

    var relto = db.query('relation', [{ 'type': 'from_me', 'ref': 'source', 'alias': 'other', 'field': 'recid' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'id', field: 'id' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'status', field: 'status' },
											{ 'type': 'from_me', 'ref': 'source', alias: 'title', field: 'title' },
											'recid'
										],
							vv.where({ 'target': request.recid, 'relationship': 'related' })
						);

    if (relto && (relto.length > 0))
    {
        bug.related = bug.related || [];
        bug.related = bug.related.concat(this.cleanRelList(relto, already));
    }

    bug.related = bug.related || [];

	var commits = db.query('vc_changeset', ['commit'],
						   vv.where( { 'item': request.recid } )
						   );

	bug.commits = commits || [];

	var idToDescriptor = null;

	var repInfo = {};
	try{
		for ( var i = bug.commits.length - 1; i >= 0; --i ){
			csid = bug.commits[i].commit;
			var matches = csid.match(/^(.+):(.+)/);
			var reponame;
			var needRepoName = false;
			var repoid;

			if(matches){
				repoid = matches[1];
				csid = matches[2];
			}
			else{
				repoid = request.repo.repo_id;
			}
			
			if(!repInfo[repoid] && (repoid==request.repo.repo_id)){
				repInfo[repoid] = {
					checkRepo: this.request.repo,
					branchesInfo: getBranchesInfo(this.request.repo),
					name: this.request.repoName
				};
			}
			else if(!repInfo[repoid]){
				noCache = true; 	// don't cache, we depend on external repos

				if (! idToDescriptor)
				{
					idToDescriptor = {};
					var descs = sg.list_descriptors();

					for ( var dn in descs )
					{
						var descriptor = descs[dn];
						var id = descriptor.repo_id;
						if (! idToDescriptor[id])
							idToDescriptor[id] = dn;
					}
				}

				reponame = idToDescriptor[repoid] || null;

				var opened;
				if (reponame)
				{
					try
					{
						opened = sg.open_repo(reponame);
						repInfo[repoid] = {
							opened: opened,
							checkRepo: opened,
							branchesInfo: getBranchesInfo(opened),
							name: reponame
						};
					}
					catch(e)
					{
						vv.log("unable to open " + reponame + ": " + e.toString());
						opened = null;
					}
				}

				if (! opened)
				{
					bug.commits.splice(i, 1);
					continue;
				}
				needRepoName = true;
			}
			else{
				needRepoName = (repoid!=request.repo.repo_id);
			}

			bug.commits[i].title = csid.substring(0,10) + "…";

			if (repInfo[repoid].checkRepo)
			{
				var desc = null;
				try
				{
					desc = repInfo[repoid].checkRepo.get_changeset_description(csid);
				}
				catch (e)
				{
					desc = null;
				}

				if (! desc)
				{
					bug.commits.splice(i,1);
					continue;
				}

				bug.commits[i].revno = desc.revno;
				bug.commits[i].id = desc.revno + ":" + bug.commits[i].title;

				if (desc.comments && (desc.comments.length > 0))
					bug.commits[i].title = desc.comments[0].text;

				bug.commits[i].branches = listOpenBranchesWithChangeset(csid, repInfo[repoid].checkRepo, repInfo[repoid].branchesInfo);
			}
			
			if (needRepoName){
				bug.commits[i].commit = repInfo[repoid].name + ":" + csid;
				bug.commits[i].repoName = reponame;
				bug.commits[i].title = "(" + reponame + ") - " + bug.commits[i].title;
			}
			else{
				bug.commits[i].commit = csid;
				bug.commits[i].title = "- " + bug.commits[i].title;
			}
		}
	}
	finally
	{
		for(x in repInfo){
			if(repInfo[x].opened){
				repInfo[x].opened.close();
			}
		}
	}
	bug.commits.sort(function(x,y){
		if(x.repoName==y.repoName){
			return y.revno - x.revno;
		}
		else if(!x.repoName){
			return -1;
		}
		else if(!y.repoName){
			return 1;
		}
		else{
			return x.repoName.localeCompare(y.repoName);
		}
	});

    bug.milestones = db.query('sprint', ['*'], null, "enddate #DESC");

    var udb = new zingdb(this.request.repo, sg.dagnum.USERS);
    bug.users = udb.query('user', ['name', 'recid', 'inactive'], null, "name");

    var work = db.query('work', ['*'], vv.where({ item: request.recid }), "when");
    if (work)
        bug.work = work;

    var stamps = db.query('stamp', ['*'], vv.where({ item: request.recid }), "when");
    if (stamps)
        bug.stamps = stamps;

	if (noCache)
	{
		bug._nocache = true;
	}

    return (bug);
};

witRequest.prototype.changesetMatch = function()
{
	var recs = [];
	var csids = {};
	var q = this.params.q || '';

	try
	{
		if (q.match(/^[a-z0-9]+$/))
		{
			var hid = this.request.repo.lookup_hid(q);

			if (hid)
			{
				var desc = this.request.repo.get_changeset_description(hid);

				var rec = {
					value: hid,
					label: desc.revno + ":" + hid.substring(0,10) + "…\t"
				};

				if (desc.comments && (desc.comments.length > 0))
					rec.label += desc.comments[0].text;

				rec.when = desc.audits[0].timestamp;
				rec.who = desc.audits[0].userid;

				recs.push(rec);
			}
		}
	}
	catch (e)
	{
	}


	if ((q.length >= 3) && (recs.length == 0))
	{
		var db = new zingdb(this.request.repo, sg.dagnum.VC_COMMENTS);

		var term = q;
		if ((q.indexOf('"') < 0) && (q.indexOf(' ') < 0))
			term += "*";

		recs = db.query__fts('item', 'text', term, ['text','csid','history'], 100);

		if (this.params.userid)
		{
			for ( var i = recs.length - 1; i >= 0; --i )
			{
				var audit = vv.getFirstAudit(recs[i]);
				var who = audit.userid;
				
				if (who != this.params.userid)
				{
					recs.splice(i, 1);
				}
			}
		}

		for ( var i = 0; i < recs.length; ++i )
		{
			var rec = recs[i];
			rec.when = vv.getFirstAudit(rec).timestamp;
			delete rec.history;
			
			var desc = this.request.repo.get_changeset_description(rec.csid);
			rec.label = desc.revno + ":" + rec.csid.substring(0,10) + "…\t" + rec.text;
			rec.value = rec.csid;

			csids[rec.csid] = true;
		}
	}

	return(recs);
};

witRequest.prototype.createWorkItem = function(bug)
{
	var request = this.request;
	var ztx = null;
	
	try
	{
		var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
		
		var fields = [
			'assignee', 'reporter', 'verifier', 'title', 'description', 'status', 'estimate', 'priority', 'listorder', 'milestone'
		];

		ztx = db.begin_tx(null, request.vvuser);

		var rec = ztx.new_record('item');
		var recid = rec.recid;

		for ( var i = 0; i < fields.length; ++i )
		{
			var fn = fields[i];

			if (typeof(bug[fn]) != 'undefined')
				rec[fn] = bug[fn];
			else if (rec[fn])
				delete rec[fn];
		}

		if (bug.comment)
		{
			var cmt = ztx.new_record('comment');
			cmt.text = bug.comment;
			cmt.who = request.vvuser;
			cmt.item = recid;
		}

		if (bug.work)
		{
			if (bug.work.amount)
			{
				var newWork = ztx.new_record("work");
				newWork.amount = bug.work.amount;
				newWork.when = bug.work.when;
				newWork.item = request.recid;
			}
			if (bug.work.estimate != undefined)
				rec.estimate = bug.work.estimate;
		}
		
		if (bug.stamps)
		{
			for (i = 0; i < bug.stamps.length; i++)
			{
				var stamp = bug.stamps[i];
				
				// Don't add new stamps that have their delete flag set.
				// The UI shouldn't pass these back to the server, but check here
				// just in case
				if (stamp._delete == "true")
					continue;
					
				var newStamp = ztx.new_record("stamp");
				newStamp.name = stamp.name;
				newStamp.item = recid;
			}
		}

		var addRel = function(relname, list, fromMe)
		{
			list = list || [];
			for ( var i = 0; i < list.length; ++i )
			{
				var rel = list[i];
				var otherid = rel.other;

				if (rel._delete)   // X-ed out during bug creation
					continue;

				var newrel = ztx.new_record('relation');
				newrel.relationship = relname;
				newrel.source = fromMe ? recid : otherid;
				newrel.target = fromMe ? otherid : recid;
			}
		};

		var addFromMe = function(relname, list)
		{
			addRel(relname, list, true);
		};

		var addToMe = function(relname, list)
		{
			addRel(relname, list, false);
		};

		addFromMe('blocks', bug.blocking);
		addToMe('blocks', bug.blockers);
		addFromMe('duplicateof', bug.duplicateof);
		addToMe('duplicateof', bug.duplicates);
		addFromMe('related', bug.related);
		
		bug.commits = bug.commits || [];

		for ( i = 0; i < bug.commits.length; ++i )
		{
			if (bug.commits[i]._delete)
				continue;

			var commit = bug.commits[i].commit;
				
			commit = this.normalizeCommitAssociation(this.request.repoName, commit);

			var newrec = ztx.new_record('vc_changeset');
			newrec.commit = commit;
			newrec.item = recid;
		}

		vv.txcommit(ztx);
		ztx = null;

		return( recid );
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.normalizeCommitAssociation = function(homeRepo, commit)
{
	if (! this.repoIds)
		this.repoIds = {};

	var result = commit;

	// check for redundant remote commit
	var matches = commit.match(/^(.+):(.+)$/);

	if (matches)
	{
		var repoName = matches[1];
		var ccsid = matches[2];

		if (repoName == homeRepo)
			result = ccsid;
		else if (this.repoIds[repoName])
			result = this.repoIds[repoName] + ":" + ccsid;
		else
		{
			var trepo = null;

			try
			{
				var id = null;

				var descs = sg.repoid_to_descriptors(repoName);

				for ( desc in descs )
				{
					this.repoIds[desc] = repoName;

					if (desc == homeRepo)
						return(ccsid);
					
					id = repoName;
				}
				
				if (! id)
				{
					trepo = sg.open_repo(repoName);
				
					if (trepo && trepo.repo_id)
						id = trepo.repo_id;
				}
				
				if (id)
				{
					result = id + ":" + ccsid;
					this.repoIds[repoName] = id;
				}
				else
					throw("Unable to match repo name " + repoName);
			}
			finally
			{
				if (trepo)
					trepo.close();
			}
		}
	}

	return( result );
};

witRequest.prototype.updateWorkItem = function (bug)
{
    var request = this.request;
    var ztx = null;

    try
    {
        var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

        var csid = bug._csid || null;

        var fields = [
			'id', 'assignee', 'reporter', 'verifier', 'title', 'description', 'status', 'estimate', 'priority', 'listorder', 'milestone'
		];

        ztx = db.begin_tx(csid, request.vvuser);

        var rec = ztx.open_record('item', request.recid);

        for (var i = 0; i < fields.length; ++i)
        {
            var fn = fields[i];

            if (typeof (bug[fn]) != 'undefined')
                rec[fn] = bug[fn];
            else if (rec[fn])
                delete rec[fn];
        }

        if (bug.comment)
        {
            var cmt = ztx.new_record('comment');
            cmt.text = bug.comment;
            cmt.who = request.vvuser;
            cmt.item = request.recid;
        }

        if (bug.stamps)
        {
            for (i = 0; i < bug.stamps.length; i++)
            {
                var stamp = bug.stamps[i];
                if (stamp.recid)
                {
                    if (stamp._delete == "true")
                        ztx.delete_record("stamp", stamp.recid);
                }
                else
                {
                    // Don't add new stamps that have their delete flag set.
                    if (stamp._delete == "true")
                        continue;

                    var newStamp = ztx.new_record("stamp");
                    newStamp.name = stamp.name;
                    newStamp.item = request.recid;
                }
            }
        }

        if (bug.work)
        {
            if (bug.work.amount)
            {
                var newWork = ztx.new_record("work");
                newWork.amount = bug.work.amount;
                if (!bug.work.when)
                {
                    bug.work.when = new Date().getTime();
                }
                newWork.when = bug.work.when;
                newWork.item = request.recid;
            }
            if (bug.work.estimate != undefined)
                rec.estimate = bug.work.estimate;           
        }
       
        var fromMe = db.query('relation', ['*'], vv.where({ 'source': request.recid }));
        var toMe = db.query('relation', ['*'], vv.where({ 'target': request.recid }));

        var blockers = {};
        var blocking = {};
        var duplicateof = {};
        var related = {};
        var duplicates = {};
        var toDelete = [];   // relations to delete in a separate, current transaction

        for (i = 0; i < fromMe.length; ++i)
        {
            var rel = fromMe[i];
            rel.existing = true;
            rel.other = rel.target;

            if (rel.relationship == 'blocks')
            {
                blocking[rel.other] = rel;
            }
            else if (rel.relationship == 'duplicateof')
            {
                duplicateof[rel.other] = rel;
            }
            else if (rel.relationship == 'related')
            {
                related[rel.other] = rel;
            }
        }
        for (i = 0; i < toMe.length; ++i)
        {
            var rel = toMe[i];
            rel.existing = true;
            rel.other = rel.source;

            if (rel.relationship == 'blocks')
            {
                blockers[rel.other] = rel;
            }
            else if (rel.relationship == 'related')
            {
                related[rel.other] = rel;
            }
            else if (rel.relationship == 'duplicateof')
            {
                duplicates[rel.other] = rel;
            }
        }

        var updateAgainst = function (fieldName, existing, relationship, isSource)
        {
            bug[fieldName] = bug[fieldName] || [];

            for (var i = 0; i < bug[fieldName].length; ++i)
            {
                var rel = bug[fieldName][i];
                var otherid = rel.other;

                if (existing[otherid]) // no change, skip
                {
                    if (rel._delete)
                    {
                        toDelete.push(existing[otherid].recid);
                        delete existing[otherid];
                    }
                }
                else
                {
                    existing[otherid] = rel;
                }
            }

            for (var recid in existing)
            {
                var rec = existing[recid];

                if ((!rec.existing) && (!rec._delete))
                {
                    var newrec = ztx.new_record('relation');
                    newrec.relationship = relationship;
                    newrec.source = isSource ? request.recid : recid;
                    newrec.target = isSource ? recid : request.recid;
                }
            }
        };

        updateAgainst('blocking', blocking, 'blocks', true);
        updateAgainst('blockers', blockers, 'blocks', false);
        updateAgainst('duplicateof', duplicateof, 'duplicateof', true);
        updateAgainst('duplicates', duplicates, 'duplicateof', false);
        updateAgainst('related', related, 'related', true);

        var recs = db.query('vc_changeset', ['commit', 'recid'],
							vv.where({ 'item': request.recid })
						   );
        var already = {};

        for (i = 0; i < recs.length; ++i)
        {
			var commit = this.normalizeCommitAssociation(this.request.repoName, recs[i].commit);
			already[commit] = recs[i].recid;
		}

        if (bug.commits)
        {
            for (i = 0; i < bug.commits.length; ++i)
            {
                var commit = bug.commits[i].commit;
                var todelete = bug.commits[i]._delete || false;

                commit = this.normalizeCommitAssociation(this.request.repoName, commit);

                if (todelete)
                {
                    if (already[commit])
                        ztx.delete_record('vc_changeset', already[commit]);
                }
                else if (!already[commit])
                {
                    var newrec = ztx.new_record('vc_changeset');
                    newrec.commit = commit;
                    newrec.item = request.recid;
                }
            }
        }
        
        vv.txcommit(ztx);
        ztx = null;

        db.merge(request.vvuser);

        if (toDelete.length > 0)
        {
            ztx = db.begin_tx(null, request.vvuser);

            for (i = 0; i < toDelete.length; ++i)
                ztx.delete_record('relation', toDelete[i]);

            vv.txcommit(ztx);
            ztx = null;
        }

        return (OK);
    }
    finally
    {
        if (ztx)
            ztx.abort();
    }
};

witRequest.prototype.updateLinks = function(rectype, data) {
	var ztx = false;

	var request = this.request;

	try
	{
		var linkinfo = this.linkfields[request.linkname];

		if (! linkinfo)
		{
			return( badRequestResponse() );
		}

		var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);
		ztx = db.begin_tx(null, request.vvuser);

		var linklist;

		if (data.linklist)
			linklist = data.linklist;
		else
			linklist = [ data ];

		if (linkinfo.isFromMe && ! linkinfo.isMultiple)
			return( this.setSingleReference(this.request.repo, db, request.vvuser, request.recid, request.linkname, linkinfo, linklist) );
		else if (linkinfo.isFromMe && linkinfo.isMultiple)
			return( this.setOutgoingLinks(this.request.repo, db, request.vvuser, request.recid, request.linkname, linkinfo, linklist) );
		else if ((! linkinfo.isFromMe) && ! linkinfo.isMultiple)
			return( this.setSingleInboundReference(this.request.repo, db, request.vvuser, request.recid, request.linkname, linkinfo, linklist) );
		else 
			return( this.setInboundLinks(this.request.repo, db, request.vvuser, request.recid, request.linkname, linkinfo, linklist) );
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};

witRequest.prototype.setSingleReference = function(repo, db, user, recid, linkname, linkinfo, linklist)
{
	var ztx = null;

	try
	{
		ztx = db.begin_tx(null, user);

		var rec = ztx.open_record('item', recid);

		if (linklist.length == 0)
			delete rec[linkname];
		else if (linklist.length == 1)
			rec[linkname] = linklist[0].recid;
		else
			throw "Only one " + linkname + " may be set.";

		ztx.commit();
		ztx = null;
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};


witRequest.prototype.setOutgoingLinks = function(repo, db, user, recid, linkname, linkinfo, linklist)
{
	var ztx = null;

	var params = linkinfo.values || {};

	params[ linkinfo.linkSource ] = recid;

	try
	{
		var recs = db.query(linkinfo.linkType, ['*'], vv.where( params ) );

		var already = vv.buildLookup(recs, linkinfo.linkTarget);
		var newlist = vv.buildLookup(linklist);
		var todelete = [];
		var toadd = [];

		for ( var f in already )
			if (! newlist[f])
				todelete.push(f);

		for ( f in newlist )
			if (! already[f])
				toadd.push(f);

		if ((toadd.length + todelete.length) > 0)
			ztx = db.begin_tx(null, user);

		for ( var i = 0; i < todelete.length; ++i )
		{
			var itemrecid = todelete[i];
			var linkrecid = already[itemrecid].recid;

			ztx.delete_record(linkinfo.linkType, linkrecid);
		}

		for ( i = 0; i < toadd.length; ++i )
		{
			var itemrecid = toadd[i];
			var fields = {};

			fields[linkinfo.linkSource] = recid;
			fields[linkinfo.linkTarget] = itemrecid;

			var rec = ztx.new_record(linkinfo.linkType);

			vv.extend(rec, fields, linkinfo.values);
		}

		if (ztx)
			ztx.commit();

		ztx = null;
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}

	return(OK);
};


witRequest.prototype.setInboundLinks = function(repo, db, user, recid, linkname, linkinfo, linklist)
{
	var ztx = null;

	try
	{
		if (linkinfo.linkType)
		{
			var params = linkinfo.values || {};

			params[ linkinfo.linkTarget ] = recid;

			var recs = db.query(linkinfo.linkType, ['*'], vv.where( params ) );

			var already = vv.buildLookup(recs, linkinfo.linkSource);

			var lookupField = 'recid';

			if (linkinfo.linkSource && ! linkinfo.otherType)
				lookupField = linkinfo.linkSource;

			var newlist = vv.buildLookup(linklist, lookupField);
			var todelete = [];
			var toadd = [];

			for ( var f in already )
				if (! newlist[f])
					todelete.push(f);

			for ( f in newlist )
				if (! already[f])
					toadd.push(f);

			if ((toadd.length + todelete.length) > 0)
				ztx = db.begin_tx(null, user);

			for ( var i = 0; i < todelete.length; ++i )
			{
				var itemrecid = todelete[i];
				var linkrecid = already[itemrecid].recid;

				ztx.delete_record(linkinfo.linkType, linkrecid);
			}

			for ( i = 0; i < toadd.length; ++i )
			{
				var itemrecid = toadd[i];
				var fields = {};

				fields[linkinfo.linkTarget] = recid;
				fields[linkinfo.linkSource] = itemrecid;

				var rec = ztx.new_record(linkinfo.linkType);

				vv.extend(rec, fields, linkinfo.values);
			}
		}
		else
		{
			for ( var i = 0; i < linklist.length; ++i )
			{
				var details = linklist[i];
				var rec = null;

				if (! ztx)
					ztx = db.begin_tx(null, user);

				if (details.recid)
					rec = ztx.open_record(linkinfo.otherType, details.recid);
				else
				{
					rec = ztx.new_record(linkinfo.otherType);
					delete details.rectype;

					vv.extend(rec, details);
				}

				rec[linkinfo.otherLinkName] = recid;
			}
		}

		if (ztx)
			ztx.commit();
		ztx = null;

		return(OK);
	}
	catch (e)
	{
		vv.log(e.toString());
		return( badRequestResponse() );
	}
	finally
	{
		if (ztx)
			ztx.abort();
	}
};


witRequest.prototype.retrieveLink = function(rectype) {
	var request = this.request;

	var db = new zingdb(this.request.repo, sg.dagnum.WORK_ITEMS);

	var linkinfo = this.linkfields[request.linkname];

	if (! linkinfo)
	{
		return( badRequestResponse() );
	}

	var records = [];

	if (linkinfo.linkType && linkinfo.otherType)
	{
		var qfield = linkinfo.isFromMe ? linkinfo.linkSource : linkinfo.linkTarget;
		var p = {};
		p[qfield] = request.recid;

		var recs = db.query(linkinfo.linkType, ['*'], vv.where(p));

		var otherq = linkinfo.isFromMe ? linkinfo.linkTarget : linkinfo.linkSource;

		var recids = [];

		for ( var i = 0; i < recs.length; ++i )
			recids.push( recs[i][otherq] );
			
		p = {};
		p.recid = recids;

		var pq = vv.where(p);

		records = db.query(linkinfo.otherType, ['*'], pq);
	}
	else
	{
		var rec = db.get_record(rectype, request.recid, ['*']);

		var lf = {};
		lf[request.linkname] = linkinfo;

		this.fetchLinkRecords(records, rec, lf, db, true);
	}

	return(jsonResponse(records));
};


var workitemLinkFields = 
	{
		"comments": {
			isMultiple: true,
			isFromMe: false,
			otherType: 'comment',
			otherLinkName: 'item'
		},
		"work": {
			isMultiple: true,
			isFromMe: false,
			otherType: 'work',
			otherLinkName: 'item'
		},
		"milestone": {
			isMultiple: false,
			isFromMe: true,
			otherType: 'sprint'
		},
		"attachments": {
			isMultiple: true,
			isFromMe: false,
			otherType: 'attachment',
			otherLinkName: 'item'
		},
		"duplicateof" : {
			isMultiple: true,
			isFromMe: true,
			otherType: 'item',
			linkType: 'relation',
			linkSource: 'source',
			linkTarget: 'target',
			values: {
				relationship: "duplicateof"
			}
		},
		"blockeditems": {
			isMultiple: true,
			isFromMe: true,
			otherType: 'item',
			linkType: 'relation',
			linkSource: 'source',
			linkTarget: 'target',
			values: {
				"relationship": "blocks"
				}
		},
		"changesets": {
			isMultiple: true,
			isFromMe: false,
			linkType: 'vc_changeset',
			linkSource: 'commit',
			linkTarget: 'item'
		},
		"blockingitems": {
			isMultiple: true,
			isFromMe: false,
			otherType: 'item',
			linkType: 'relation',
			linkSource: 'source',
			linkTarget: 'target',
			values: {
				"relationship": "blocks"
				}
		}

	};

var sprintLinkFields = 
{
	"items": {
		isMultiple: true,
		isFromMe: false,
		otherType: 'item',
		otherLinkName: 'milestone'
	}	
};

registerRoutes({
    "/repos/<repoName>/workitems.json": {
        "GET": {
            onDispatch: function (request)
            {
                // this try/catch will go away; leaving it here for now to try and catch intermittent failure in testing
                try
                {
                    var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                    return (cc.get_response
					(
						function (request)
						{
						    var wir = new witRequest(request, workitemLinkFields);
						    var results = wir.fetchRecords("item");
						    return results;
						},
						this, request)
					);
                }
                catch (e)
                {
                    vv.log("exception in workitems.json:", e.toString());
                    throw (e);
                }
            }
        },

        "POST": {
            requireUser: true,
            onJsonReceived: function (request, newrec)
            {
                var wir = new witRequest(request);

                if (!newrec.reporter)
                    newrec.reporter = request.vvuser;

                var txt = wir.createWorkItem(newrec);

                return (textResponse(txt));

                //	                    return (wir.addRecord('item', newrec));

            }
        }

    },

    "/repos/<repoName>/workitem/<recid>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request, workitemLinkFields);
					    var results = wir.fetchRecords("item");

					    if (!results)
					        throw notFoundResponse();

					    if ((results.length == 1) && (!wir.params._includelinked))
					        results = results[0];

					    return results;
					},
					this, request)
				);
            }
        },
        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, newrec)
            {
                var wir = new witRequest(request);

                return (wir.updateRecord("item", newrec));
            }
        },

        "DELETE": {
            requireUser: true,
            onDispatch: function (request)
            {
                var wir = new witRequest(request);

                return (wir.deleteRecord("item"));
            }
        }
    },

    "/repos/<repoName>/workitem-full/<recid>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request, workitemLinkFields);

					    var result = wir.fullWorkItem();
					    if (!result)
					        throw notFoundResponse();

					    if (result._nocache)
					    {
					        delete result._nocache;
					        throw jsonResponse(result);
					    }

					    return result;
					},
					this, request)
				);
            }
        },
        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, newrec)
            {
                var wir = new witRequest(request);

                return (wir.updateWorkItem(newrec));
            }
        }
    },

    "/repos/<repoName>/wicommitsearch.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.VERSION_CONTROL, sg.dagnum.VC_COMMENTS]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request);

					    if (!wir.params.q)
					        throw notFoundResponse();

					    var recs = wir.changesetMatch();

					    return recs;
					},
					this, request)
				);
            }
        }
    },

    "/repos/<repoName>/workitem-fts.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request);

					    if (!wir.params.term)
					        throw notFoundResponse();

					    var recs = wir.search(['id', 'title']);

					    var results = [];

					    for (var i = 0; i < recs.length; ++i)
					    {
					        results.push(
									{
									    'value': recs[i].recid,
									    'label': recs[i].id + " " + recs[i].status + " " + recs[i].title
									}
								);
					    }

					    return results;
					},
					this, request)
				);
            }
        }
    },

    "/repos/<repoName>/workitem-details.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request);

					    wir.params._ids = wir.params._ids || '';

					    var recs = wir.witDetails();

					    var results = [];

					    for (var i = 0; i < recs.length; ++i)
					    {
					        results.push(
									{
									    'value': recs[i].recid,
									    'label': recs[i].id + " " + recs[i].status + " " + recs[i].title
									}
								);
					    }

					    return results;
					},
					this, request)
				);
            }
        }
    },

    "/repos/<repoName>/workitem-full/<recid>/work.json": {
        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, data)
            {
                var wir = new witRequest(request);

                return (wir.updateWorkitemWork(data));
            }
        }
    },

    "/repos/<repoName>/milestone/<recid>/<linkname>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var wir = new witRequest(request, sprintLinkFields);

                return (wir.retrieveLink('sprint'));
            }
        },

        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, data)
            {
                var wir = new witRequest(request);

                return (wir.updateLinks('sprint', data));
            }
        }
    },

    "/repos/<repoName>/workitem/<recid>/comments.json": {
        "GET": {
            onDispatch: function (request)
            {
                var wir = new witRequest(request);
                return (wir.getComments(request.recid));
            }
        },
        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, data)
            {
                var wir = new witRequest(request);
                return (wir.addComment(data));
            }
        }
    },

    "/repos/<repoName>/workitem/<recid>/work.json": {
        "GET": {
            onDispatch: function (request)
            {
                request.linkname = 'work';
                var wir = new witRequest(request, workitemLinkFields);

                return (wir.retrieveLink('item'));
            }
        },

        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, data)
            {
                var wir = new witRequest(request);

                return (wir.addWork(data));
            }
        }
    },

    "/repos/<repoName>/workitem/<recid>/<linkname>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var wir = new witRequest(request, workitemLinkFields);

                return (wir.retrieveLink('item'));
            }
        },

        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, data)
            {
                var wir = new witRequest(request, workitemLinkFields);

                return (wir.updateLinks('item', data));
            }
        }
    },

    "/repos/<repoName>/workitem/<recid>/attachment/<attachid>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
                var attRec = db.get_record('attachment', request.attachid);

                if (!attRec)
                    return (notFoundResponse());

                request.hid = attRec.data;
                request.filename = attRec.filename;
                request.contenttype = attRec.contenttype;

                if (vv.getQueryParam('show', request.queryString) == "true")
                {
                    return (dispatchBlobDownload(request, false));
                }

                return (dispatchBlobDownload(request, true));
            }
        },

        "DELETE": {
            onDispatch: function (request)
            {
                var ztx = null;
                var from = request.vvuser || vv.getQueryParam('From', request.queryString) || null;  // may want to roll this in to vvuser

                try
                {
                    var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
                    var attRec = db.get_record('attachment', request.attachid);

                    if (!attRec)
                        return (notFoundResponse());

                    ztx = db.begin_tx(null, from);
                    ztx.delete_record('attachment', request.attachid);

                    vv.txcommit(ztx);
                    ztx = false;

                    return (OK);
                }
                finally
                {
                    if (ztx)
                        ztx.abort();
                }
            }
        }
    },

    "/repos/<repoName>/workitem/<recid>/attachments": {
        "POST": {
            onMimeFileReceived: function (request, path, filename, mimetype)
            {
                var from = request.vvuser || vv.getQueryParam('From', request.queryString) || null;

                if (!from)
                {
                    var udb = new zingdb(request.repo, sg.dagnum.USERS);

                    var userrecs = udb.query('user', ['recid'], "name == 'nobody'");

                    if (userrecs && (userrecs.length == 1))
                        from = userrecs[0].recid;
                }

                var ztx = false;

                try
                {
                    var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

					var item = db.get_record('item', request.recid);

					if (! item)
						return( notFoundResponse() );

                    ztx = db.begin_tx(null, from);

                    var attrec = ztx.new_record('attachment');
                    attrec.contenttype = mimetype;

                    attrec.filename = filename;

                    attrec.data = path;

                    attrec.item = request.recid;
                    var recid = attrec.recid;

                    vv.txcommit(ztx);
                    ztx = false;

                    try
                    {
                        var folder = null;
                        var ind = path.indexOf(filename);
                        if (ind > 0)
                        {
                            folder = path.substring(0, ind - 1);
                        }

                        sg.fs.remove(path);
                        if (folder)
                            sg.fs.rmdir(folder);
                    }
                    catch (e)
                    {
                        vv.log("error attempting to clean up attachment temp file:", e.toString());
                    }

                    return (textResponse(recid));
                }
                finally
                {
                    if (ztx)
                        ztx.abort();
                }
            }
        }
    },

    "/repos/<repoName>/workitem-batch-update.json": {
        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, mods)
            {
                var wir = new witRequest(request);

                var results = wir.batchUpdateItems(mods);

                return (jsonResponse(results));
            }
        }
    },

    "/repos/<repoName>/workitem/<recid>/navigation.json": {

        "GET": {
            enableSessions: true,
            onDispatch: function (request)
            {
                var nav = {};
                //get the prev and next recids from the session
                var session = vvSession.get();
              
                var sessionToken = session[FILTER_RESULTS_TOKEN];
                if (sessionToken)
                {
                    var curIndex = vv.inArray(request.recid, sessionToken);
                    if (curIndex > 0)
                    {
                        nav.prev = sessionToken[curIndex - 1];
                    }
                    if (curIndex < sessionToken.length - 1)
                    {
                        nav.next = sessionToken[curIndex + 1];
                    }
                }
                return jsonResponse(nav);
            }
        }
    },
    "/repos/<repoName>/workitem/bugid/<recid>": {

        "GET": {
            onDispatch: function (request)
            {

                var zd = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
                var recs = zd.query('item', ['recid'], vv.where({ 'id': request.recid }));
                if (recs.length != 1)
                    return (notFoundResponse());

                request.recid = recs[0].recid;
                var response = {};
                var url = request.linkRoot + "/repos/" + request.repoName + "/workitem.html?recid=" + request.recid;
                response.statusCode = "307 Temporary Redirect";
                response.sz = sg.to_sz("<html><body>Redirecting to <a href=\"" + url + "\">" + url + "</a>...</body></html>");
                response.onChunk = chunk_sz;
                response.finalize = free_sz;
                response.headers = ({
                    "Content-Length": response.sz.length,
                    "Content-Type": CONTENT_TYPE__HTML,
                    "Location": url
                });
                return response;
            }
        }

    },
    "/repos/<repoName>/workitem/<recid>/hover.json": {
        "GET": {
            onDispatch: function (request)
            {
                var zd = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);

                if (request.recid && (request.recid.length < 65))
                {
                    var recs = zd.query('item', ['recid'], vv.where({ 'id': request.recid }));
                    if (recs.length != 1)
                        return (notFoundResponse());

                    request.recid = recs[0].recid;
                }

                var rec = zd.get_record("item", request.recid, ["recid", "id", "assignee", "reporter", "verifier", "title", "status", "estimate", "priority", "milestone", "created_date",
					{ "type": "from_me", "ref": "milestone", "alias": "milestonename", "field": "name" }
					]);

                if (rec.assignee)
                    rec.assigneename = vv.lookupUser(rec.assignee, request.repo);
                if (rec.reporter)
                    rec.reportername = vv.lookupUser(rec.reporter, request.repo);
                if (rec.verifier)
                    rec.verifiername = vv.lookupUser(rec.verifier, request.repo);

                var response = jsonResponse(rec);
                return (response);
            }
        }
    },

    "/repos/<repoName>/milestones.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request);
					    var results = wir.fetchRecords("sprint");
					    return results;
					},
					this, request)
				);
            }
        },
        "POST": {
            requireUser: true,
            onJsonReceived: function (request, newrec)
            {
                var wir = new witRequest(request);

                return (wir.addRecord('sprint', newrec));
            }
        }
    },

    "/repos/<repoName>/milestone/<recid>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request);

					    var results = wir.fetchRecords("sprint");

					    if (!results)
					        throw notFoundResponse();

					    if (results.length == 1)
					        results = results[0];

					    return results;
					},
					this, request)
				);
            }
        },

        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, newrec)
            {
                var wir = new witRequest(request);

                return (wir.updateRecord("sprint", newrec));
            }
        },

        "DELETE": {
            requireUser: true,
            onDispatch: function (request)
            {
                var wir = new witRequest(request);

                return (wir.deleteRecord("sprint"));
            }
        }
    },

    "/repos/<repoName>/workitem/<recid>/commit/<csid>.json": {
        "POST": {
            requireUser: true,
            onDispatch: function (request)
            {
                var wir = new witRequest(request);
                var recid = wir.addCommit();

                return (textResponse(recid));
            }
        },

        "DELETE": {
            requireUser: true,
            onDispatch: function (request)
            {
                var wir = new witRequest(request);
                wir.deleteCommit();

                return (OK);
            }
        }
    },

    "/repos/<repoName>/work/<recid>.json": {
        "PUT": {
            requireUser: true,
            onJsonReceived: function (request, newrec)
            {
                var wir = new witRequest(request);

                return (wir.updateWork(newrec));
            }
        },

        "DELETE": {
            requireUser: true,
            onDispatch: function (request)
            {
                var wir = new witRequest(request);

                return (wir.deleteRecord("work"));
            }
        }
    },

    "/repos/<repoName>/workitem/stamps.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL]);
                return (cc.get_response
				(
					function (request)
					{
					    var wir = new witRequest(request);

					    var results = null;
					    try
					    {
					        results = wir.searchStamps();
					    }
					    catch (ex)
					    {
					        if (ex.toString().indexOf("sqlite") >= 0)
					        {
					            throw jsonResponse({ error: "syntax error in search string" });
					        }
					        else
					        {
					            throw (ex);
					        }
					    }

					    if (!results)
					        throw notFoundResponse();

					    return results;
					},
					this, request)
				);
            }
        }
    }
});
