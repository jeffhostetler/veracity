function hook(params)
{
	var result = {};
	var repos = {};
	var trx  = {};
	var dbs = {};

	var _sqlEscape = function (st)
	{
		if (st === null)
			return ('#NULL');
		else if (!st)
			return ("''");
		else if (typeof (st) != 'string')
			return (st);

		st = st.replace(/'/, "''");

		return ("'" + st + "'");
	};

	try
	{
		var commitRepoid = params.descriptor_name;
		var seen = {};

		for ( var i = 0; i < params.wit_ids.length; ++i )
		{
			var wid = params.wit_ids[i];
			var commitId = params.csid;
			var assocRepoid = commitRepoid;

			var matches = wid.match(/^(.+):(.+)$/);
			if (matches)
			{
				commitId = params.repo_id + ":" + params.csid;
				assocRepoid = matches[1];
				wid = matches[2];
			}

			var ww = _sqlEscape(wid);

			var where = "(id == " + ww.toUpperCase() + ") || (recid == " + ww.toLowerCase() + ")";

			if (! repos[assocRepoid])
			{
				var uid = null;

				if (assocRepoid == commitRepoid)
					uid = params.userid || null;

				var r = sg.open_repo(assocRepoid);

				if (! r)
					throw("unable to open repo " + assocRepoid);
				repos[assocRepoid] = r;
				dbs[assocRepoid] = new zingdb(r, sg.dagnum.WORK_ITEMS);
				trx[assocRepoid] = dbs[assocRepoid].begin_tx(null, uid);
			}

			var repo = repos[assocRepoid];
			var db = dbs[assocRepoid];
			var ztx = trx[assocRepoid];

			var recs = db.query('item', ['recid'], where, null, 1);

			if (recs.length != 1)
				throw("Can't find work item " + wid + " in " + assocRepoid);

			var key = assocRepoid + ":" + recs[0].recid;

			if (! seen[key])
			{
				seen[key] = true;

				var assoc = ztx.new_record('vc_changeset');

				assoc.commit = commitId;
				assoc.item = recs[0].recid;
			}
		}

		for ( var rn in trx )
		{
			var ztx = trx[rn];

			var ct = ztx.commit();

			if (ct.errors)
				throw(sg.to_json(ct.errors));

			delete trx[rn];
		}
	}
	catch(e)
	{
		sg.log(e.toString());
		result.error = e.toString();
	}
	finally
	{
		for ( var rn in trx )
		{
			var ztx = trx[rn];
			ztx.abort();
		}

		for ( rn in repos )
			repos[rn].close();
	}

	return(result);
}