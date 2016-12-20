var vvWikiModule = {
	defaultHomePage: function()
	{
		var page = { 'title' : 'Home' };

		page.text = "This is the stock Veracity wiki home page.  Feel free to edit it to suit your needs - " +
			"include links to your most-visited pages, team-specific information and links, cookie recipes, " +
			"whatever you'd like.\n\n" +
			"The sidebar to the right lists your existing pages; use the search box to quickly filter that list.\n\n" +
			"You may also, of course, create a [[New Page]] at any time.\n";

		return(page);
	},

	checkPage: function(request)
	{
		try
		{
			var db = new zingdb(request.repo, sg.dagnum.WIKI);
			db.merge(request.vvuser || null);

			var leaves = db.get_leaves();
		}
		catch(e)
		{
			leaves = [];
		}

		if (leaves.length > 1)
		{
			return (preconditionFailedResponse("Wiki DAG has multiple heads"));
		}
		else if (leaves.length < 1)
			return null;

		var csid = leaves[0];

		var w = vv.where( { "title": request.pagename } );

		var recs = db.query('page', ['text','title','recid','history'], w);
		var rec = null;

		if (recs && (recs.length > 0))
			rec = recs[0];

		if (rec)
			rec._csid = csid;

		return( rec );
	},

	getPage: function(request)
	{
		var rec = this.checkPage(request);

		if (! rec)
		{
			if (request.pagename == 'Home')
				return(this.defaultHomePage());
			else
				return(null);
		}

		var text = rec.text;

		var descriptors = [];

		var rdescs = sg.list_descriptors();

		for ( var desc in rdescs )
			descriptors[ rdescs[desc].repo_id ] = desc;

		text = text.replace(/(^|[^\\])@([a-g_\-0-9]+):([a-g0-9]+)/g,
							function( str, preamble, id, csid ) {
								var descriptor = descriptors[id] || id;
								return( preamble + "@" + descriptor + ":" + csid );
							}
						   );

		rec.text = text;

		var separator = "\n== ZING_MERGE_CONCAT ==\n";

		if (rec.text.match(separator))
		{
			var rtext = "# Conflicting Edits Detected\n\n" +
				"Look below for any \"Conflicts with:\" headings.  Below each of these is a version of this wiki page " +
				"that could not be auto-merged.  Please choose the combination of text you'd like to keep, and delete the rest " +
				"(including this heading and paragraph)\n\n" +
				rec.text;  // todo: elaborate
			rtext = rtext.replace(/== ZING_MERGE_CONCAT ==/g, "# Conflicts with:");

			rec.text = rtext;
		}

		var a = vv.getLastAudit(rec);

		if (a)
		{
			rec.when = a.timestamp;
			rec.who = a.username || "";
		}

		return(rec);
	},

	postWikiPage: function(request, newrec)
	{
		if (newrec.title.match(/[\[\]^@\\]/))
			throw new Error("Page titles must not contain: [, ], ^, @ or \\");

		var db = new zingdb(request.repo, sg.dagnum.WIKI);
		db.merge(request.vvuser || null);

		var ztx = null;
		var rec = null;

		var ids = [];

		var rdescs = sg.list_descriptors();

		for ( var desc in rdescs )
			ids[desc] = rdescs[desc].repo_id;

		try
		{
			var csid = newrec._csid || null;
			delete newrec._csid;

			var oldrecs = db.query('page', ['recid'], vv.where({ 'title': newrec.title }));

			if (oldrecs && (oldrecs.length > 0))
			{
				var oldrec = oldrecs[0];
				var newid = newrec.recid || '';

				if (newid != oldrec.recid)
				{
					throw new Error("duplicate title");
				}
			}

			ztx = db.begin_tx(csid, request.vvuser);
			if (newrec.recid)
				rec = ztx.open_record('page', newrec.recid);
			else
			{
				rec = ztx.new_record('page');
				newrec.recid = rec.recid;
			}

			var text = newrec.text;

			text = text.replace(/(^|[^\\])@([a-z_\-0-9]+):([a-g0-9]+)/g,
								function( str, preamble, descriptor, csid ) {
									var id = ids[descriptor] || descriptor;
									return( preamble + "@" + id + ":" + csid );
								}
							   );

			rec.title = newrec.title;
			rec.text = text;

			vv.txcommit(ztx);
			ztx = null;
		}
		finally
		{
			if (ztx)
				ztx.abort();
		}

		return( newrec.recid );
	},

	getDiff: function(request)
	{
		var cacheRequest = {
			'requestMethod': 'get',
			'uri': request.repoName + '/wikidiff/' + request.page + '/' + request.csid1 + '/' + request.csid2,
			'queryString': request.queryString || '',
			'headers': {},
			'repo': request.repo
		};

		var fetchFunc = function(request)
		{
			var db = new zingdb(request.repo, sg.dagnum.WIKI);

			var recs = db.query('page', ['recid','history'], vv.where( { 'title': request.page } ));

			if ((! recs) || (recs.length == 0))
				return(null);

			var recid = recs[0].recid;

			var a1, a2;
			var history = recs[0].history;

			for ( var i = 0; i < history.length; ++i )
			{
				if (history[i].csid == request.csid1)
					a1 = history[i].audits[0];
				else if (history[i].csid == request.csid2)
					a2 = history[i].audits[0];
			}

			var rec1 = db.get_record('page', recid, ['text','history'], request.csid1);
			var rec2 = db.get_record('page', recid, ['text','history'], request.csid2);

			var fn1 = sg.fs.tmpdir() + "/" + sg.gid();
			var fn2 = sg.fs.tmpdir() + "/" + sg.gid();

			sg.file.write(fn1, rec1.text);
			sg.file.write(fn2, rec2.text);

			var getDesc = function(a)
			{
				return( a.username + " / " + (new Date(parseInt(a.timestamp))).toLocaleDateString() );
			};

			var label1 = getDesc(a1);
			var label2 = getDesc(a2);

			return sg.diff_files(fn1, label1, fn2, label2, request.page);
		};

		var cc = new textCache(cacheRequest, [sg.dagnum.WIKI], CONTENT_TYPE__TEXT);
		return (cc.get_text(fetchFunc, this, request));
	}
};


registerRoutes({
	"/repos/<repoName>/wiki/workitem-status/<bugid>.json" : {
		"GET": {
			onDispatch: function(request) {

				var cc = new textCache(request, [sg.dagnum.WORK_ITEMS], CONTENT_TYPE__TEXT);
				return (cc.get_response
				(
					function(request)
					{
						var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
						var bid = request.bugid.toUpperCase();
						var recs = db.query('item', ['status'], vv.where( { 'id': bid } ));

						if (recs && (recs.length > 0))
							return recs[0].status || "";
						else
							throw notFoundResponse();
					},
					this, request)
				);
			}
		}
	},

	"/repos/<repoName>/wiki/workitem/<bugid>.json" : {
		"GET": {
			onDispatch: function(request) {
				var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
				var bid = request.bugid.toUpperCase();
				var recs = db.query('item', ['recid','status'], vv.where( { 'id': bid } ));

				if (recs && (recs.length > 0))
				{
					var url = request.linkRoot + "/repos/" + request.repoName + "/workitem.html?recid=" + recs[0].recid;
					var response = errorResponse(
						"307 Temporary Redirect",
						request,
						"Your browser should redirect you to <a href=\"" + url + "\">" + url + "</a>. (If it doesn't follow the link.)",
						true);
					response.headers["Location"] = url;
					return(response);
				}
				else
					return( notFoundResponse() );
			}
		}

	},

	"/repos/<repoName>/wiki/page.json": {
		"GET": {
			onDispatch: function (request) {

				var cc = new textCache(request, [sg.dagnum.WIKI]);
				return (cc.get_response
				(
					function(request)
					{
						var pagename = vv.getQueryParam('title', request.queryString);

						if (! pagename)
							throw badRequestResponse(request, "No page title was supplied");

						request.pagename = pagename;

						var rec = vvWikiModule.getPage(request);
						if (! rec)
							throw notFoundResponse();

						return rec;
					},
					this, request)
				);
			}
		},

		"HEAD": {
			onDispatch: function (request) {

				var cc = new textCache(request, [sg.dagnum.WIKI]);
				var t = cc.get_text
				(
					function(request)
					{
						var pagename = vv.getQueryParam('title', request.queryString);

						if (! pagename)
							throw badRequestResponse(request, "");

						request.pagename = pagename;

						var there = !! vvWikiModule.checkPage(request);
						if (! there)
						{
							return "404";
						}
						else
						{
							return "200";
						}
					},
					this, request
				);

				if (t == "200")
					return( textResponse("") );
				else
					return( notFoundResponse() );
			}
		},

		"POST": {
			requireUser: true,
			onJsonReceived: function (request, newrec) {
				try
				{
					var resp = vvWikiModule.postWikiPage(request, newrec);
					return( textResponse(resp) );
				}
				catch (e)
				{
					if (e.message == "duplicate title")
						return errorResponse(
							"409 Conflict",
							request,
							"A wiki page with that title already exists. Please choose another title and try again.");
					else if (e.message.match(/must not contain/))
						return( badRequestResponse( request, e.message ) );
					else
						throw(e);
				}
			}
		},
	   "DELETE" : {
		   requireUser: true,
			onDispatch: function(request) {
				var pagename = vv.getQueryParam('title', request.queryString);

				if (! pagename)
					return( badRequestResponse(request, "No page title was supplied") );

				request.pagename = pagename;

				var p = vvWikiModule.getPage(request);

				if (! p)
					return(notFoundResponse(request, "The page was not found - it may have already been deleted or renamed.") );

				var recid = p.recid;

				var db = new zingdb(request.repo, sg.dagnum.WIKI);
				var ztx = null;

				try
				{
					ztx = db.begin_tx(null, request.vvuser);

					ztx.delete_record('page', recid);
					vv.txcommit(ztx);
					ztx = null;
				}
				finally
				{
					if (ztx)
						ztx.abort();
				}

				return(OK);
			}
	   }
   },

   "/repos/<repoName>/wiki/diff.txt": {
	   "GET": {
		   onDispatch: function(request) {

				var cc = new textCache(request, [sg.dagnum.WIKI], CONTENT_TYPE__TEXT);
				return (cc.get_response
				(
					function(request)
					{
						request.page = vv.getQueryParam('page', request.queryString);
						if (! request.page)
							throw badRequestResponse(request, "Page title was not supplied");

						request.csid1 = vv.getQueryParam('csid1', request.queryString);
						if (! request.csid1)
							throw badRequestResponse(request, "First csid was not supplied");

						request.csid2 = vv.getQueryParam('csid2', request.queryString);
						if (! request.csid2)
							throw badRequestResponse(request, "Second csid was not supplied");

						return vvWikiModule.getDiff(request);
					},
					this, request)
				);
			}
		}
   },

   "/repos/<repoName>/wiki/pages.json": {
	   "GET" : {
		   onDispatch: function(request) {

			   var cc = new textCache(request, [sg.dagnum.WIKI]);
				return (cc.get_response
				(
					function(request)
					{
					   var db = new zingdb(request.repo, sg.dagnum.WIKI);

					   var term = vv.getQueryParam('q', request.queryString);
					   var recs = null;

					   if (term)
					   {
						   term = term.replace(/'/g, '"');
						   term = term.ltrim();

						   if (term.indexOf('"') != 0 && term.indexOf(" ") < 0)
							   term += "*";
						   recs = db.query__fts('page', 'title', term, ['*']);
					   }
					   else
						   recs = db.query('page',  ['title','last_timestamp'], null, 'last_timestamp #DESC');

					   if (! recs)
						   recs = [];

					   return recs;
					},
					this, request)
				);
		   }
	   }
   },

	"/repos/<repoName>/wiki/image": {
		"POST": {
			onMimeFileReceived: function (request, path, filename, mimetype)
			{
				var from = request.vvuser || vv.getQueryParam('From', request.queryString) || null;

				if (! from)
					return (unauthorizedResponse());

				var res = {
					'mimeType': mimetype,
					'filename': filename,
					'recid': null
				};

				var ztx = false;

				try
				{
					var db = new zingdb(request.repo, sg.dagnum.WIKI);

					ztx = db.begin_tx(null, from);

					var attrec = ztx.new_record('image');
					attrec.contenttype = mimetype;

					attrec.filename = filename;

					attrec.data = path;

					var recid = attrec.recid;

					vv.txcommit(ztx);
					ztx = false;

					db.merge(from);

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

					res.recid = recid;
				}
				finally
				{
					if (ztx)
						ztx.abort();
				}

				return( textResponse(sg.to_json(res)) );
			}
		}
	},


	"/repos/<repoName>/wiki/image/<recid>/<filename>": {
		"GET": {
			onDispatch: function (request)
			{
				var db = new zingdb(request.repo, sg.dagnum.WIKI);
				var attRec = db.get_record('image', request.recid);

				if (!attRec)
					return (notFoundResponse());

				request.hid = attRec.data;
				request.filename = attRec.filename;
				request.contenttype = attRec.contenttype;

				return (dispatchBlobDownload(request, true));
			}
		}
	}

});

