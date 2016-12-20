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

var buildActivity =
{
	name: function()
	{
		return("builds");
	},

	dagsUsed: function()
	{
		return( [ sg.dagnum.BUILDS ]);
	},

	getActivity: function(request, max, userid)
	{
		var results = [];

		try
		{
			var db = new zingdb(request.repo, sg.dagnum.BUILDS);

			var count, s, where = null;

			var includeStatuses = db.query('status', ['recid'], vv.where({'show_in_activity': true}));

			if (includeStatuses != null && includeStatuses.length > 0)
			{
				var statlist = [];
				
				for ( var i = 0; i < includeStatuses.length; ++i )
					statlist.push(includeStatuses[i].recid);
				where = vv.where({ 'status_ref' : statlist });
			}

			/* last 'max' build events, with history, include only those having show_in_activity == true */
			var recs = db.query('build', ['recid',
										 {
											 'type' : 'from_me',
											 'ref' : 'buildname_ref',
											 'field' : 'name'
										 },
										 {
											 'type' : 'from_me',
											 'ref' : 'buildname_ref',
											 'field' : 'branchname'
										 },
										 {
											 'type' : 'from_me',
											 'ref' : 'series_ref',
											 'field' : 'name',
											 'alias' : 'seriesname'
										 },
										 {
											 'type' : 'from_me',
											 'ref' : 'status_ref',
											 'field' : 'name',
											 'alias' : 'statusname'
										 },
										 {
											 'type' : 'from_me',
											 'ref' : 'environment_ref',
											 'field' : 'name',
											 'alias' : 'envname'
										 },
										 'history'
										],
							   where, 'updated #DESC', max);

			var already = [];

			vv.each(recs,
					function(i, rec)
					{
						var audit = vv.getFirstAudit(rec);
						if (! audit)
							return;

						var act = {
							action: rec.seriesname + ' Build: ' + rec.statusname,
							link: '/build.html?recid=' + rec.recid,
							what: rec.envname + ' - ' + rec.name + ' (' + rec.branchname + ')',
							who: audit.userid,
							when: audit.timestamp
						};

						var key = rec.seriesname + rec.statusname + rec.name;

						if (already[key])
						{
							already[key].what = rec.envname + ", " + already[key].what;
						}
						else
						{
							already[key] = act;
							results.push(act);
						}
					}
				   );
		}
		catch (e)
		{
			sg.log('urgent', 'Failed to pull build activity: ' + e.toString());
		}

		return( results );
	}
};

activityStreams.addSource(buildActivity);

