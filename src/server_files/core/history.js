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
    "/repos/<repoName>/history.json": {
        "GET":
		{
		    enableSessions: true,
		    onDispatch: function (request)
		    {
		        var results = null;
		        var session = vvSession.get();

		        try
		        {
                    var pageToken = vv.getQueryParam('tok', request.queryString);
		            var tokenName = "history_token" + pageToken;
		            if (vv.queryStringHas(request.queryString, "more"))
		            {
		                if (!session || !session[tokenName])
		                {
		                    return notFoundResponse(request, "Your session has timed out. You will need to refresh the history page.");
		                }
		                results = vvHistory.fetchMore(request, session, tokenName);
		            }
		            else
		            {
		                results = vvHistory.getHistory(request, session, tokenName);
		            }

		            if (results)
		            {
		                return jsonResponse(results);
		            }
		            return jsonResponse({ error: "The history search returned no results." });
		        }
		        catch (ex)
		        {
		            if (ex.toString().indexOf("not be found") >= 0)
		            {
		                return jsonResponse({ error: "The requested object could not be found. Verify that the path exists in the repository." });
		            }
		            else if (ex.toString().indexOf("The stamp was not found") >= 0)
		            {
		                return jsonResponse({ error: "The requested stamp was not found" });
		            }
		            else
		            {
		                throw (ex);
		            }
		        }
		    }
		}
    }
});

var vvHistory =
{
    getHistory: function (request, session, pageToken)
    {
        var results = null;

        var partTmp = vv.getQueryParams(request.queryString) || {};
        var parts = {};
        var max = null

        vv.each(partTmp, function (i, v)
        {
            if (i == 'path')
            {
                parts[i] = v;
            }
            else
            {
                parts[i] = v.replace(/'/g, "''");
            }
        });

        if (parts['max'] !== undefined)
            max = parseInt(parts['max'], 10);

        var path = null;
        if (parts['path'])
        {
            path = parts['path'];
            path.ltrim("/");
            if (path.indexOf("@") != 0)
            {
                path = "@/" + path;
            }
        }
        var merges = null;
        if (parts['hidemerges'] && path && path.length)
        {
            var b = parts['hidemerges'];
            if (b == "true")
                merges = true;
            else
                merges = false;
        }
        var from = null;
        var to = null;
        if (parts['from'])
        {
            from = new Date(parseInt(parts['from'])); ///.strftime("%Y-%m-%d");
            
            from = parts['from'];
        }
        if (parts['to'])
        {
            to = new Date(parseInt(parts['to'])); ///.strftime("%Y-%m-%d");
         
            to = parts['to'];
        }
        results = request.repo.history(max,
								   parts['user'] || null,
								   parts['stamp'] || null,
								   from,
								   to,
                                   parts['branch'] || null,
                                   path || null,
                                   merges || false,
								   true
								  );

        if (results)
        {
            var token = session[pageToken];
            if (results.next_token)
            {
                session[pageToken] = results.next_token;
            }
            else
            {
                delete session[pageToken];
            }
            return ({ items: results.items, more: results.next_token != null });
        }
        else
        {
            delete session[pageToken];
        }

    },
    fetchMore: function (request, session, pageToken)
    {
        var max = vv.getQueryParam('max', request.queryString);

        var token = session[pageToken];

        if (max)
            max = parseInt(max, 10);

        if (token)
        {
            var results = request.repo.history_fetch_more(token, max);
            if (results)
            {
                if (results.next_token)
                {
                    session[pageToken] = results.next_token;
                }
                else
                {
                    delete session[pageToken];
                }
                return ({ items: results.items, more: results.next_token != null });
            }
            else
            {
                delete session[pageToken];
            }
        }

    }

};
