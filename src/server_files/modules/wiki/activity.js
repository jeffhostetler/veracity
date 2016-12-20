/*
Copyright 2011-2013 SourceGear, LLC

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

var wikiActivitySource = {
	name: function()
	{
		return("wikiActivitySource");
	},

	dagsUsed: function()
	{
		return( [sg.dagnum.WIKI, sg.dagnum.USERS] );
	},

	coalesce: function(recs)
	{
		recs.sort(
			function(a,b)
			{
				return( b.when - a.when );
			}
		);

		for ( var i = recs.length - 1; i > 0; --i )
		{
			var prev = recs[i - 1];
			var cur = recs[i];

			if ((prev.action == cur.action) && (prev.link == cur.link) && (prev.who == cur.who))
				recs.splice(i, 1);
		}
	},

	getActivity: function(request, max, userid)
	{
		var repo = request.repo;
		var db = new zingdb(repo, sg.dagnum.WIKI);
		var vvUserIdNOBODY = "g3141592653589793238462643383279502884197169399375105820974944592";

		if (request.vvuser)
			db.merge(request.vvuser);

		var enough = false;
		var done = false;
		var sofar = 0;

		var results = [];

		while (results.length < max)
		{
			var recentMods = db.query__recent('page', ['*'],
											  null,
											  max, sofar);

			if ((! recentMods) || (recentMods == 0))
				break;

			sofar += recentMods.length;

			var recentRecords = {};

			for ( var i = 0; i < recentMods.length; ++i )
				recentRecords[recentMods[i].recid] = true;

			for ( var recid in recentRecords )
			{
				var lastpage = null;
				var skipFirst = false;

				var history = db.query__recent('page', ['*'], vv.where( { 'recid' : recid } ), max + 1);

				if (history.length > max)
					skipFirst = true;

				var title = history[0].title;

				var first = true;

				var ends = history.length - 1;

				for ( var j = ends; j >= 0; --j )
				{
					var thispage = history[j];

					var record = {
						what: thispage.title,
						title: thispage.title,
						who: thispage.userid,
						when: thispage.timestamp,
						description: thispage.title
					};

					if (record.who == vvUserIdNOBODY)
						continue;

					if (first)
						record.action = "Created Wiki page";
					else
						record.action = "Edited Wiki page";

					if (lastpage)
					{
						if (lastpage.title != thispage.title)
						{
							record.action = "Renamed Wiki page";
							record.what += " (was " + lastpage.title + ")";
						}
					}

					if (lastpage)
					{
						var req = {
							'repoName': request.repoName,
							'repo': repo,
							'page': title,
							'csid1': lastpage.csid,
							'csid2': thispage.csid
						};

						try
						{
							record.atomContent = vvWikiModule.getDiff( req );
						}
						catch (e)
						{
							vv.log(e.toString());
							record.atomContent = '';
						}
					}

					lastpage = thispage;

					record.link = '/wiki.html?page=' + encodeURIComponent(title);

					if (! (first && skipFirst))
						results.push(record);

					first = false;
				}
			}

			this.coalesce(results);
		}

		return(results);
	}
};

activityStreams.addSource(wikiActivitySource);
