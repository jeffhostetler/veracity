function hook(params)
{
	var result = {};
	var repos = {};
	var trx  = {};
	var dbs = {};
	
	var where = '';
	var term = params.text;

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
	
	var fields = ["id", "title"];
	
	var foundRecs = {};

	this.repo = sg.open_repo(params.descriptor_name);
	
	var db = new zingdb(this.repo, sg.dagnum.WORK_ITEMS);

	var items = [];
	
	for ( var i = 0; i < fields.length; ++i )
	{
			searchTerm = fields[i] + " #MATCH " + _sqlEscape(term + '*');
			var recs = db.query('item', ['*'], searchTerm, 'status');

			for ( var j = 0; j < recs.length; ++j )
					foundRecs[ recs[j].recid ] = recs[j];
	}

	

	for ( var recid in foundRecs)
			items.push(foundRecs[recid]);
	result["items"] = items;
	this.repo.close();
	return(result);
}