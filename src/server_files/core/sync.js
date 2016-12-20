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

/**
 * This file contains all the routes used by the HTTP SG_sync_client to do push, pull, and clone.
 */

function fragballResponse(request, data)
{
	var tmpdir = sg.fs.tmpdir();
	var fragball_file_path = tmpdir + "/" + sg.sync_remote.request_fragball(request.repo, tmpdir, data);
	var response = fileResponse(fragball_file_path, true);
	return response;
}

registerRoutes({

    "/version.txt":
	{
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        return textResponse(sg.version);
		    }
		}
	},

    "/repos/<repoName>/sync.fragball":
	{
	    /**
	    * Handles a fragball request (typically for pull or clone).
	    *   - Will have either:
	    *     - A zero-length message body, indicating a request for the leaves of every DAG, or
	    *     - A jsonified vhash in the message body, whose format is described by SG_sync_remote__request_fragball
	    */
	    "POST":
		{
		    availableOnReadonlyServer: true,
		    availableToInactiveUser: false,

		    onDispatch: function (request)
		    {
		        try
		        {
		            if (request.headers["Content-Length"] == 0)
		            {
		                return fragballResponse(request);
		            }
		            return null; // Allow onJsonReceived to handle the request after we've got the body.
		        }
		        catch (ex)
		        {
		            return json500ResponseForException(request, ex, [208,275]);
		        }
		    },

		    onJsonReceived: function (request, data)
		    {
		        try
		        {
		            return fragballResponse(request, data);
		        }
		        catch (ex)
		        {
		            return json500ResponseForException(request, ex, [208,275]);
		        }
		    }
		}
	},

    /* This isn't under the sync folder, and isn't directly push/pull related, but it's here because
    * it's implemented by SG_sync_remote and it's part of the sync vtable so it can be called transparently
    * for local or remote repositories. */
    "/repos/<repoName>/heartbeat.json":
	{
	    /**
	    * Retrieve basic repository information that can be cached, just to indicate that a repository is available.
	    */
	    "GET":
		{
		    onDispatch: function (request)
		    {
		        var cc = new textCache(request);
		        return cc.get_response(function(repo) {return sg.sync_remote.heartbeat(repo);}, this, request.repo);
		    }
		}
	},

    "/repos/<repoName>/sync.json":
	{
	    /**
	    * Retrieve repository information before starting a push or pull.
	    */
	    "GET":
		{
		    availableToInactiveUser: false,
		    onDispatch: function (request)
		    {
		        /* NOTE: You can't cache this response with textCache because it returns a list
		        of all DAGs present in the repository. TextCache needs a list of applicable
		        DAGs to properly invalidate itself, and no such list is possible. */
		        var bIncludeBranchesAndLocks = vv.queryStringHas(request.queryString, "details");
		        var b_include_areas = vv.queryStringHas(request.queryString, "areas");
		        var response = sg.sync_remote.get_repo_info(request.repo, bIncludeBranchesAndLocks, b_include_areas);
		        return jsonResponse(response);
		    }
		},

	    /**
	    * Handles a push_begin request.
	    *   - Will always have an empty message body
	    */
	    "POST":
		{
		    availableToInactiveUser: false,
		    onDispatch: function (request)
		    {
		        /* This is a push_begin request. We'll create a staging area and return the new push ID. */
		        var pushID = sg.sync_remote.push_begin();

		        return jsonResponse({ id: pushID });
		    }
		}

	},

    "/repos/<repoName>/sync/<syncid>":
	{
	    /**
	    * Retrieve the status for a pending push.
	    */
	    "GET":
		{
		    availableToInactiveUser: false,
		    onDispatch: function (request)
		    {
		        var response = sg.sync_remote.check_status(request.repo, request.syncid);
		        return jsonResponse(response);
		    }
		},

	    /**
	    * Add to a pending push. The PUT file is a fragball containing dagnodes and/or blobs to add.
	    * The status of the push is returned, the same as if another GET had been performed.
	    */
	    "PUT":
		{
		    availableToInactiveUser: false,
		    onDispatch: function (request)
		    {
		        if (request.headers["Content-Length"] == 0)
		        {
		            // Bad request. Return HTTP 400.
		            return errorResponse(STATUS_CODE__BAD_REQUEST, request);
		        }
		        return null; // Allow onFileReceived to handle the request after we've got the body.
		    },
		    onFileReceived: function (request, file_path)
		    {
		        var push_result = sg.sync_remote.push_add(request.repo, request.syncid, file_path);
		        return jsonResponse(push_result);
		    }
		},

	    /**
	    * Commit a push.
	    */
	    "POST":
		{
		    availableToInactiveUser: false,
		    onDispatch: function (request)
		    {
		        if (request.headers["Content-Length"] != 0)
		        {
		            // Bad request. Return HTTP 400.
		            return errorResponse(STATUS_CODE__BAD_REQUEST, request);
		        }
		        var commit_result = sg.sync_remote.push_commit(request.repo, request.syncid);
		        return jsonResponse(commit_result);
		    },
		    onRequestAborted: function (request)
		    {
				sg.sync_remote.push_end(request.syncid);
				sg.log("push aborted server-side: " + request.syncid);
		    }
		},

	    /**
	    * Finish/cleanup a push.
	    */
	    "DELETE":
		{
		    onDispatch: function (request)
		    {
		        sg.sync_remote.push_end(request.syncid);
		        return OK;
		    }
		}

	},

    "/repos/<repoName>/sync/incoming.json":
	{
	    /**
	    * Return changeset info for the requested nodes, which were determined to be "incoming"
	    */
	    "POST":
		{
		    availableOnReadonlyServer: true,
		    availableToInactiveUser: false,
		    onDispatch: function (request)
		    {
		        if (request.headers["Content-Length"] == 0)
		        {
		            // Bad request. Return HTTP 400.
		            return errorResponse(STATUS_CODE__BAD_REQUEST, request);
		        }
		        return null; // Allow onJsonReceived to handle the request after we've got the body.
		    },
		    onJsonReceived: function (request, data)
		    {
		        var response = sg.sync_remote.get_dagnode_info(request.repo, data);
		        return jsonResponse(response);
		    }
		}
	},

    "/repos/<repoName>/sync/clone":
	{
	    /**
	    * Handles a push_clone_begin request.
	    *   - The message body contains the source repo info json.
	    */
	    "POST":
		{
		    noRepo: true,
		    availableOnReadonlyServer: false,
		    onDispatch: function (request)
		    {
		        if (request.headers["Content-Length"] == 0)
		        {
		            // Bad request. Return HTTP 400.
		            return errorResponse(STATUS_CODE__BAD_REQUEST, request);
		        }
		        return null; // Allow onJsonReceived to handle the request after we've got the body.
		    },
		    onJsonReceived: function (request, data)
		    {
		        /* This is a push_clone_begin request. We'll verify that the repo name is valid and available,
		        * then create a clone area on disk and return the new clone ID. */
		        try
		        {
		            var cloneID = sg.sync_remote.push_clone_begin(request.repoName, data);
		            return jsonResponse({ id: cloneID });
		        }
		        catch (ex)
		        {
		            return json500ResponseForException(request, ex, [154,205,208,233,275]);
		        }
		    }
		}

	},

    "/repos/<repoName>/sync/clone/<cloneid>":
	{
	    /**
	    * Upload a pending clone. The PUT file is the clone fragball.
	    */
	    "PUT":
		{
		    noRepo: true,
		    availableOnReadonlyServer: false,

		    "onRequestAborted": function (request)
		    {
				sg.sync_remote.push_clone_abort(request.cloneid);
				sg.log("push clone aborted server-side: " + request.cloneid);
		    },

		    onDispatch: function (request)
		    {
		        if (request.headers["Content-Length"] == 0)
		        {
		            // Bad request. Return HTTP 400.
		            return errorResponse(STATUS_CODE__BAD_REQUEST, request);
		        }
		        return null; // Allow onFileReceived to handle the request after we've got the body.
		    },

		    onFileReceived: function (request, file_path)
		    {
		        try
		        {
		            var clone_result = sg.sync_remote.push_clone_commit(request.cloneid, file_path);
		            return jsonResponse(clone_result);
		        }
		        catch (ex)
		        {
		            return json500ResponseForException(request, ex);
		        }
		    }
		},

	    /**
	    * Cleanup an aborted clone. Only necessary if clone_begin was successful but clone_commit will never be called.
	    */
	    "DELETE":
		{
		    noRepo: true,
		    availableOnReadonlyServer: false,
		    onDispatch: function (request)
		    {
		        sg.sync_remote.push_clone_abort(request.cloneid);
				sg.log("push clone aborted client-side: " + request.cloneid);
		        return OK;
		    }
		}
	}

});
