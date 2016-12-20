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
function cloneResponse(request, name)
{
    var response = {
        statusCode: STATUS_CODE__OK,
        headers: { "Content-Length": 0 },
        onFinish: function () 
        { 
        	 	var ret = sg.clone__exact(request.repoName, name); 
		   	if (ret)
			{
				sg.log(ret);
			}
        }
    };

    return response;
}

function getDefaultPageResponse(request)
{
	
	var response;
	var cookies = request.cookies;
	var defaultRepoName =  getDefaultRepoName(request);
	
     if (defaultRepoName)
     {             
        var url = request.linkRoot + "/repos/" + vvUiTemplateResponse.prototype.escape(defaultRepoName) + "/home.html";
     }
     else
     {
     	var url = request.linkRoot + "/repos.html";
     }
    
     response = {};
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
registerRoutes({
    
    "/": {
        "GET": {
            onDispatch: function (request)
            {
                return getDefaultPageResponse(request);
            }
        }
    },
    "/repos.html": {
        "GET": {
            onDispatch: function (request)
            {                
                var vvt = new vvUiTemplateResponse(request, "core", "Repositories");

                return (vvt);
            }
        }
    },
    "/repos/<repoName>/repos.html": {
        "GET": {
            onDispatch: function (request)
            {
                var vvt = new vvUiTemplateResponse(request, "core", "Repositories");

                return (vvt);
            }
        }
    },
     "/repos/<repoName>/home.html": {
        "GET": {
            onDispatch: function (request)
            {
                var vvt = new vvUiTemplateResponse(request, "core", "Home");

                return (vvt);
            }
        }
    },
    "/repos/manage.json": {
        "GET": {
            onDispatch: function (request)
            {
                var descriptors = sg.list_all_available_descriptors();
               
				var defaultRepo = sg.get_local_setting("server/repos/default");

                responseObj = { "has_excludes": descriptors.has_excludes };
                var descriptorsObj = {};
                for (var descriptorName in descriptors.descriptors)
                {
					var repo = null;
                    var descriptor = descriptors.descriptors[descriptorName];
                    try
                    {
						if (descriptor.status == "0")
						{
							repo = sg.open_repo(descriptorName);
							var nbranches = repo.named_branches();
							var rhids = repo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
                            var def = false;
                            if (defaultRepo)
                            {
							    def = (descriptorName == defaultRepo);
                            }
							descriptorsObj[descriptorName] = { "descriptor": descriptor, "status": descriptor.status, "hids": rhids, "branches": nbranches, "defaultRepo": def };
						}
						else
						{
							descriptorsObj[descriptorName] = { "descriptor": descriptor, "status": descriptor.status };
						}
                    }
					catch (e)
					{
						sg.log("debug", "error opening repository '" + descriptorName + "': " + e.toString());
					}
                    finally
                    {
						if (repo != null)
							repo.close();
                    }
                }

                responseObj.descriptors = descriptorsObj;
                return jsonResponse(responseObj);
            }
        }
    },
    "/repos/manage/<repoName>.json": {
        "PUT": {
            availableOnReadonlyServer: false,
            onJsonReceived: function (request, data)
            {
                var oldname = request.repoName;
                //if the old repo name was the default repo, make it the default again
                var ret = sg.rename_repo(request.repoName, data.name);
                if (!ret)
                {
                    var defaultRepo = sg.get_local_setting("server/repos/default");
                    if (defaultRepo == oldname)
                    {
                        sg.set_local_setting("server/repos/default", data.name);
                    }

                }

                return ret;
            }
        }
    },

    "/repos.json": {
        "GET": {
            onDispatch: function (request)
            {
				var defaultRepo = sg.get_local_setting("server/repos/default");

                var descriptors = sg.list_descriptors();
                var responseObj = [];

				for (var desc in descriptors)
				{
					obj = { 'descriptor': desc };
					if (desc == defaultRepo) { obj["defaultRepo"] = true; };
					responseObj.push( obj );
				}

                return jsonResponse(responseObj);
            }
        },

        "POST": {
            availableOnReadonlyServer: false,
            onJsonReceived: function (request, data)
            {
                if (data.shareUsers)
                {
                    return sg.vv2.init_new_repo( { "repo" : data.name, "no-wc" : true, "shared-users" : data.shareUsers } )
                }
                else
                {
                    return sg.vv2.init_new_repo( { "repo" : data.name, "no-wc" : true } );
                }
            }
        }

    },

    "/repos/<repoName>/users.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cc = new textCache(request, [sg.dagnum.USERS]);
                var response = cc.get_response(
                    function(repo) 
                    {
                        var db = new zingdb(repo, sg.dagnum.USERS);
                        return db.query("user", ["*"]);
                    }, this, request.repo);

                return response;
            }
        }
    },
    "/repos/<repoName>/fileclasses.json": {
        "GET": {
            onDispatch: function (request)
            {
                var fileClasses = request.repo.file_classes();
                if (fileClasses)
                {
                    return jsonResponse(fileClasses);
                }
                return (notFoundResponse());
            }
        }
    },
    "/repos/<repoName>.$": {
        "GET": {
            onDispatch: function (request)
            {
                var vvt = new vvUiTemplateResponse(request, "core", "Home", null, "home.html");

                return (vvt);
            }
        }
    },
    "/repos/<repoName>.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cacheFunc = function(request)
                {
                    var responseObj = {};
                    responseObj.descriptor = request.repo.name;
                    responseObj.RepoID = request.repo.repo_id;
                    responseObj.AdminID = request.repo.admin_id;
                    responseObj.HashMethod = request.repo.hash_method;

                    if (vv.queryStringHas(request.queryString, "details"))
                    {
                        var leaves = request.repo.fetch_dag_leaves(sg.dagnum.VERSION_CONTROL);
                        var namedbranches = request.repo.named_branches();

                        //make sure we retrieve all the branch heads info even
                        //if the head isn't a dag leaf
                        if (namedbranches)
                        {
                            for (var branch in namedbranches.values)
                            {
                                var needed = true;
                                for (var i = 0; i < leaves.length; i++)
                                {
                                    if (leaves[i] == branch)
                                    {
                                        needed = false;
                                        break;
                                    }
                                }
                                if (needed)
                                    leaves.push(branch);
                            }
                        }
                        var hidHash = {};
                        vv.each(leaves, function (i, v)
                        {
                            try
                            {
                                hidHash[v] = request.repo.get_changeset_description(v);
                            }
                            catch (ex)
                            {
                                hidHash[v] = { "changeset_id": v };
                            }
                        });

                        for (var hid in hidHash)
                        {
                            if (hidHash[hid].audits)
                            {
                                for (var j = 0; j < hidHash[hid].audits.length; ++j)
                                {
                                    var un = vv.lookupUser(hidHash[hid].audits[j].userid, request.repo);

                                    if (un)
                                        hidHash[hid].audits[j].username = un;
                                }
                            }
                        }

                        responseObj.hids = hidHash;
                        responseObj.branches = namedbranches;
                    }

                    return responseObj;
                };

                var cc = new textCache(request, [sg.dagnum.VERSION_CONTROL, sg.dagnum.VC_COMMENTS, sg.dagnum.USERS, sg.dagnum.VC_BRANCHES]);
                return (cc.get_response(cacheFunc, this, request));
            }
        }
    },

    "/repos/<repoName>/clone.json":
	{
	    /* Ask the server to make a clone of a repo it already has.
	    * Not to be confused with pushing/pulling an entire clone, which is handled in sync.js. */
	    "POST": {
	        availableOnReadonlyServer: false,
	        onJsonReceived: function (request, data)
	        {
	            return cloneResponse(request, data.name);
	        }
	    }
	},

	"/defaultrepo.json":
	{
		"POST": {
	        availableOnReadonlyServer: false,
		
			onJsonReceived: function(request, data)
			{
				var repoName = data.repo;

				if (! repoName)
					return( badRequestResponse(request, "No repo name specified") );

                var descriptors = sg.list_descriptors();

				if (! descriptors[repoName])
					return( badRequestResponse( request, "Unknown repo name: " + repoName ));

				sg.set_local_setting("server/repos/default", repoName);

				return( OK );
			}
		}
	}

});
