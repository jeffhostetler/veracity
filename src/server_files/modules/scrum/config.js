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

function Configuration(repo, user)
{
	this.repo = repo;
	this.user = user;

	this.get = function (key)
	{
	    var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
	    try
	    {
	        var recs = db.query("config", ["*"], vv.where({ "key": key }));

	        var rec = null;

	        if (recs && recs.length > 0)
	            rec = recs[0];

	        return rec;
	    }
	    catch (ex)
	    {
	        return null;
	    }

	};

	this.set = function (key, value, tx)
	{
	    var rec = this.get(key);

	    if (!tx)
	    {
	        var db = new zingdb(repo, sg.dagnum.WORK_ITEMS);
	    }

	    try
	    {
	        if (!tx)
	            var ztx = db.begin_tx(null, this.user);
	        else
	            var ztx = tx;

	        if (!rec)
	        {
	            var zrec = ztx.new_record("config");
	        }
	        else
	        {
	            var zrec = ztx.open_record('config', rec.recid);
	        }

	        zrec.key = key;
	        zrec.value = value;

	        if (!tx)
	        {
	            vv.txcommit(ztx);

	            ztx = null;
	            db.merge(this.user);
	        }
	    }
	   
	    finally
	    {
	        if (!tx)
	        {
	            if (ztx)
	                ztx.abort();
	        }
	    }
	};
}

var createOrUpdate = {
	requireUser: true,
	onJsonReceived: function(request, newrec) {
		var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
		
		var rec = null;
		
		if (request.key)
		{
			rec = db.get_record('config', request.key);
			
			if (!rec)
			{
				var records = db.query("config", ["*"], vv.where( { key: request.key } ));
					
				if (records.length > 0)
				{
					rec = records[0];
				}
			}
		}
		
		sg.log(sg.to_json(rec));
		
		if (rec)
			request.recid = rec.recid;

		var wir = new witRequest(request);

		if (!rec)
			return( wir.addRecord('config', newrec) );
		else
			return( wir.updateRecord("config", newrec) );
		
		return( badRequestResponse() );
	}
};

registerRoutes({
	"/repos/<repoName>/config.json": {
		"GET": {
			onDispatch: function(request) {

				var cc = new textCache(request, [ sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL ]);
				return (cc.get_response
				(
					function(repo)
					{
						var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
						var options = db.query("config", ["*"], null, "key #DESC",null, null);
						return options;
					}, 
					this, request.repo)
				);
			}
		},
        "POST": createOrUpdate
	},

	"/repos/<repoName>/config/<key>.json": {
		"GET": {
			onDispatch: function(request) {

				var cc = new textCache(request, [ sg.dagnum.WORK_ITEMS, sg.dagnum.VERSION_CONTROL ]);
				return (cc.get_response
				(
					function(repo)
					{
						var db = new zingdb(request.repo, sg.dagnum.WORK_ITEMS);
						var options = db.query("config", ["*"], vv.where( { key : request.key}));
						var ret = null;
						if (options.length === 0)
							ret = { key: request.key, value: ""};
						else
							ret = options[0];
							
						return ret;
					}, 
					this, request.repo)
				);
			}
		},
		"PUT": createOrUpdate,
		"DELETE": {
			requireUser: true,
			onDispatch: function(request) {
				var wir = new witRequest(request);
					
				return(wir.deleteRecord("config"));
			}
		}
	}
});
