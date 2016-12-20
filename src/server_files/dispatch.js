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


//
// The serverConf object is a copy of the "server" config setting as it existed
// at server startup, and thus contains members such as "files" and "readonly".
// 
// This object exists for settings that can and/or should be consistent over
// the life of the server. Other settings, however, should be loaded live from
// the config file(s) as needed. These settings include:
// 
//   server/repos/default - This setting can be changed from the web ui while
//                          the server is running and should not require a
//                          server restart to take affect.
// 
//   server/clone_allowed - This setting is accessed from C code, which fetches
//                          the current value from disk. Thus the current disk
//                          value should be used from SSJS too for consistency.
// 
// It may also be helpful to note that the following members get special
// treatement before being inserted (by C code) into serverConf:
// 
//   server/application_root - serverConf always holds the value being used by
//                             this server. This may not match what is in the
//                             config file, since the setting does not apply to
//                             veracity's internal servers (such as 'vv serve'
//                             and the tortoise server), but only to veracity
//                             as embedded in e.g. apache or IIS.
// 
//   server/enable_diagnostics - Forced into a boolean value.
// 
//   server/readonly - Forced into a boolean value.
// 
//   server/remote_ajax_libs - Forced into a boolean value.
// 
//   server/ssjs_mutable - Forced into a boolean value.
//

var serverConf = {};

var init = function(serverConfParam){ // init function called from C code
	serverConf = serverConfParam;
};


//
// Misc helpers
//

String.prototype.trim = function(chars) {
	var str = this.rtrim(chars);
	str = str.ltrim(chars);
	return str;

};
String.prototype.ltrim = function(chars) {
	chars = chars || "\\s";
	return this.replace(new RegExp("^[" + chars + "]+", "g"), "");
};

String.prototype.rtrim = function(chars) {
	chars = chars || "\\s";
	return this.replace(new RegExp("[" + chars + "]+$", "g"), "");
};

Array.isArray = Array.isArray || function(o) { return Object.prototype.toString.call(o) === '[object Array]'; };

var trimSlashes = function(str) {
	return str.replace(new RegExp("^\\/+", "g"), "").replace(new RegExp("\\/+$", "g"), "");
};

var firstKeyIn = function (obj) {
	for (var key in obj)
		return key;
};

var getUserHash = function(repo) {
	var y = {};
	var db = new zingdb(repo, sg.dagnum.USERS);
	var users = db.query("user", ["name", "recid"]);
	for(var i=0; i<users.length; ++i){
		y[users[i].recid] = users[i].name;
	}
	return y;
};


var STATUS_CODE__OK = "200 OK";
var STATUS_CODE__MOVED_PERMANENTLY = "301 Moved Permanently";
var STATUS_CODE__NOT_MODIFIED = "304 Not Modified";
var STATUS_CODE__BAD_REQUEST = "400 Bad Request";
var STATUS_CODE__UNAUTHORIZED = "401 Authorization Required";
var STATUS_CODE__FORBIDDEN = "403 Forbidden";
var STATUS_CODE__NOT_FOUND = "404 Not Found";
var STATUS_CODE__METHOD_NOT_ALLOWED = "405 Method Not Allowed";
var STATUS_CODE__CONFLICT = "409 Conflict";
var STATUS_CODE__PRECONDITION_FAILED = "412 Precondition Failed";
var STATUS_CODE__INTERNAL_SERVER_ERROR = "500 Internal Server Error";

var CONTENT_TYPE__ATOM = "application/atom+xml;charset=UTF-8";
var CONTENT_TYPE__FRAGBALL = "application/fragball";
var CONTENT_TYPE__JSON = "application/json;charset=UTF-8";
var CONTENT_TYPE__TEXT = "text/plain;charset=UTF-8";
var CONTENT_TYPE__JAVASCRIPT = "application/javascript;charset=UTF-8";

var CONTENT_TYPE__HTML = "text/html;charset=UTF-8";
var CONTENT_TYPE__XHTML = "application/xhtml+xml;charset=UTF-8";

var htmlContentTypeForUserAgent = function(ua) {
	return(CONTENT_TYPE__HTML);
};

var chunk_sz = function () {
	var ret = this.sz;
	this.sz = null;
	return ret;
};
var free_sz = function () {
	this.sz.fin();
	this.sz = null;
};

var jsonResponse = function(object, statusCode, headers) {
	var response = {};
	response.onChunk = chunk_sz;
	response.finalize = free_sz;
	response.sz = sg.to_json__sz(object);
	response.statusCode = statusCode || STATUS_CODE__OK;

	response.headers = headers || {};
	response.headers["Content-Length"] = response.sz.length;
	response.headers["Content-Type"] = CONTENT_TYPE__JSON;

	return response;
};

var OK = {
	statusCode: STATUS_CODE__OK,
	headers: {
		"Content-Length": 0
	}
};

var textResponse = function(text, contentType) {
	var response = {};
	response.onChunk = chunk_sz;
	response.finalize = free_sz;
	response.sz = sg.to_sz(text);
	response.statusCode = STATUS_CODE__OK;
	response.headers = ({
		"Content-Length": response.sz.length,
		"Content-Type": contentType !== undefined ? contentType : CONTENT_TYPE__TEXT
	});
	return response;
};

var fileResponse = function (path, deleteWhenDone, contentType, forceDownload, filename)
{
    var file = sg.fs.fetch_file(path, 2097152, deleteWhenDone);

    var response = {
        statusCode: STATUS_CODE__OK,
        headers: {
            "Content-Length": file.total_length
        },
        onChunk: function () { return file.next_chunk(); },
        finalize: function () { file.abort(); },
        onFinish: function () { file.abort(); }
    };

    if (contentType)
    {
        response.headers["Content-Type"] = contentType;
        response.headers["X-Content-Type-Options"] = "nosniff";
    }

    if (forceDownload)
    {
        if (filename)
            response.headers["Content-Disposition"] = "attachment; filename=" + filename;
        else
            response.headers["Content-Disposition"] = "attachment";
    }

    return response;
};

// Parameters:
//   statusCode: Http status code (for example "400 Bad Request").
//   request:    The request associated with this error response. Note: This parameter is considered
//               optional, but if it's not provided we'll be forced to return a plaintext response.
//               If provided, we'll choose between plaintext and html based on what was requested.
//   message:    Plaintext string that will be included in the response. Optional.
var errorResponse = function (statusCode, request, message) {
	statusCode = statusCode || STATUS_CODE__INTERNAL_SERVER_ERROR;
	message = message || statusCode;

	var response;

	if( request
		&& request.requestMethod === "GET"
		&& ( request.uri.slice(request.uri.length-5)===".html" || request.uri.lastIndexOf('.')<=request.uri.lastIndexOf('/') )
	)
	{
		var replacer = function(key)
		{
			if (key=="ERRORMESSAGE")
				return message;
			else if (key == "ERRORCLASS")
				return("error" + parseInt(statusCode));
		};

		// Html response.
		sg.log('debug', "Constructing html page for \""+statusCode+"\" response to GET of "+request.uri);
		response = new vvUiTemplateResponse(
			request,
			"core",
			"Veracity",
			replacer,
			"error.html");
		response.statusCode = statusCode;
	}
	else {
		// Plaintext response.
		sg.log('debug', "Constructing plaintext \""+statusCode+"\" response.");
		response = {};
		response.statusCode = statusCode;
		response.sz = sg.to_sz(message);
		response.onChunk  = chunk_sz;
		response.finalize = free_sz;
		response.headers  = ({
			"Content-Length": response.sz.length,
			"Content-Type": CONTENT_TYPE__TEXT
		});
	}

	return response;
};

/* If the error number is in verboseErrorCodeArray, the details of the error are sent back to the client.
 * Otherwise we throw, typically resulting in the generic plaintext HTTP 500 response. */
var json500ResponseForException = function(request, ex, verboseErrorCodeArray)
{
	var statusCode = STATUS_CODE__INTERNAL_SERVER_ERROR;
	
	if (verboseErrorCodeArray !== undefined)
	{
		/* When jsglue gives proper exception objects for sglib errors, this regex hack won't be necessary. */

		/* Error 140 (sglib): There is no working copy here: The path 'C:/Users/iolsen/' is not inside a working copy.
                 [1] [2]      [3] */
		var reMsg = /Error\s([0-9]{1,7})\s\((\w+)\):\s(.+)/; //
		var match = reMsg.exec(ex.message);

		if (match==null) {
			throw(ex); // If we're not explicitly handling it, we throw to get a generic response.
		}

		var objMsg = {};
		objMsg.num = parseInt(match[1],10);
		objMsg.src = match[2];
		objMsg.msg = match[3];

		if (!isNaN(objMsg.num) && verboseErrorCodeArray.indexOf(objMsg.num) != -1)
		{
			sg.log('urgent', "Exception caught during "+request.requestMethod+" request to "+request.uri+":\n"+ex.message);

			var szMsg = sg.to_json(objMsg);
			sg.log('debug', "JSON \""+statusCode+"\" response: " + szMsg);

			// JSON response
			var response = {};
			response.statusCode = statusCode;
			response.sz = sg.to_sz(szMsg);
			response.onChunk  = chunk_sz;
			response.finalize = free_sz;
			response.headers  = ({
									"Content-Length": response.sz.length,
									"Content-Type": CONTENT_TYPE__JSON
								});
		
			return response;
		}
	}

	throw(ex); // If we're not explicitly handling it, we throw to get a generic response.
};

var badRequestResponse = function (request, message) {
	return errorResponse(STATUS_CODE__BAD_REQUEST, request, message || "Bad request.");
};
var preconditionFailedResponse = function (request, message) {
	return errorResponse(STATUS_CODE__PRECONDITION_FAILED, request, message || "Precondition failed.");
};
var conflictResponse = function (request, message)
{
    return errorResponse(STATUS_CODE__CONFLICT, request, message || "Conflict.");
};
var methodNotAllowedResponse = function (request, message, allow) {
	response = errorResponse(STATUS_CODE__METHOD_NOT_ALLOWED, request, message || STATUS_CODE__METHOD_NOT_ALLOWED);
	if (allow)
		response.headers["Allow"] = allow;
	return response;
};
var notFoundResponse = function (request, message) {
	return errorResponse(STATUS_CODE__NOT_FOUND, request, message || "Not found.");
};
var unauthorizedResponse = function (request, message) {
	return errorResponse(STATUS_CODE__UNAUTHORIZED, request, message || "Authorization Required.");
};
var redirectResponse = function(request, url, message) {
	var response = errorResponse(STATUS_CODE__MOVED_PERMANENTLY, request, message || STATUS_CODE__MOVED_PERMANENTLY);

	response.headers["Location"] = request.linkRoot + (url || "/");

	return(response);
};

var getLocalRepoName = function(request) {
	if (! request.localRepoName)
	{
		var localRepo = false;

		try
		{
			localRepo = sg.open_local_repo();
			if (!! localRepo)
				request.localRepoName = localRepo.name;
			else
				request.localRepoName = "";
		}
		catch (e)
		{
			request.localRepoName = "";
		}

		if (localRepo)
			localRepo.close();
	}

	return(request.localRepoName);
};

var getDefaultRepoName = function (request) {
	//if the server was started within a working directory
	var defaultRepoName = getLocalRepoName(request);

	if(defaultRepoName) return defaultRepoName;

	var descs = sg.list_descriptors();

	//else check our last visted repos cookie to see where we were last
	var cookies = request.cookies;
	
	if (cookies && cookies["sgVisitedRepos"] && cookies["sgVisitedRepos"].length)
	{
		try
		{
			var repos = sg.from_json(decodeURIComponent(cookies["sgVisitedRepos"]));		
			defaultRepoName = repos[0];
      	}
      	catch (e)
      	{
      		defaultRepoName = "";
      	}
	}
	
	if (defaultRepoName && ! descs[defaultRepoName])
		defaultRepoName = "";

	if(defaultRepoName) return defaultRepoName;
	
	//else check the default repo config setting
	defaultRepoName = sg.get_local_setting("server/repos/default");
	
	if (defaultRepoName && ! descs[defaultRepoName])
		defaultRepoName = "";

	if (defaultRepoName) 
		return defaultRepoName;

	//else return the first descriptor in the list
	defaultRepoName = firstKeyIn(descs);

	return defaultRepoName;
};


// "routes" is a structure that stores an aggregation of all registered
// routes. It is a hierarchical map of url "parts" (ie substrings when split
// on the slashes) with the null object, a file extension, and an http verb
// included on the end as the final three "parts". For example, all of the
// following might exist in routes.
//
// routes["repos"][null]["html"]["GET"]                       // to get /repos.html
// routes["repos"][null]["json"]["GET"]                       // to get /repos.json
// routes["repos"]["*"][null]["html"]["GET"]                  // to get /repos/myrepo.html
// routes["repos"]["*"][null]["json"]["GET"]                  // to get /repos/myrepo.json
// routes["repos"]["*"]["activity"][null]["atom"]["GET"]      // to get /repos/myrepo/activity.atom
// routes["repos"]["*"]["history"][null]["html"]["GET"]       // to get /repos/myrepo/history.html
// routes["repos"]["*"]["sync"][null][undefined]["POST"]      // to post to /repos/myrepo/sync
// routes["repos"]["*"]["sync"]["*"][null][undefined]["GET"]  // to get /repos/myrepo/sync/syncid
// routes["repos"]["*"]["sync"]["*"][null][undefined]["POST"] // to post to /repos/myrepo/sync/syncid
// routes["repos"]["*"]["users"][null]["json"]["GET"]         // to get /repos/myrepo/users.json
// ... etc.
//
// Each of these would evaluate to an object containing callback functions to
// handle a request, plus an optional flag or two. Possible members include:
//
//   Flags:
//
//     availableOnReadonlyServer: bool - Can be used to enable or disable a
//         given route for when the server is in readonly mode. Defaults to
//         true for GET routes and false for all others.
//     noRepo: bool - If the <repoName> URL substitution is present, don't
//         attempt to open the repository by that name. Note that when this
//         flag is true, the availableToInactiveUser has no effect because
//         no repo is open, so no user list is available.
//
//   Functions:
//
//     onDispatch: function(request) - Called after the http request headers
//         have been received.
//     onJsonReceived: function(request, data) - Use this function when you
//         are expecting incoming json. The 'data' parameter will be the
//         object, already interpreted from the json.
//     onFileReceived: function(request, file_path) - Use this function
//         if you want the incoming message body to be stored in a file and
//         then handed to you. The file will appear in the "user temp
//         directory". If the file still exists at the original location when
//         your function returns, it will be deleted.
//     onMimeFileReceived: function(request, path, filename, mimetype) - Similar
//         to onFileReceived().
//     onRequestAborted: function(request) - Called if we are going to cancel
//         the request before it is finished being recieved. Will only be called
//         *instead of* one of the above onReceived functions. It will not be
//         called if an error is produce during or after one of them.
//
// Notice that the request object is passed to each of these functions. If you
// need to store state, to be used by subsequent calls, go ahead and store it
// in the request object.
//
// Any of the functions (except for onRequestAborted), can return a response
// object. If a response object is returned, no further calls will be made to
// any of the request handling functions (even if the request body wasn't
// finished being received).
//
// A response object can contain the following information that the framework
// will use to send the response:
//
//     statusCode: string **REQUIRED**
//     headers: object **REQUIRED** This is a JS object that acts as a key-value
//                                  hash where each key should be a HTTP header
//                                  field name. "Content-Length" is a required
//                                  key in this hash.  The value for a key can be
//                                  the string or integer to send for that header
//                                  or an array of values.  In that case, the
//                                  header will be sent multiple times to the client.
//
//     onChunk: function() **REQUIRED (unless Content-Length is 0)**
//                         Function must return a "cbuffer".
//     onFinish: function() - Called after the last chunk has been sent. If this
//                            function throws an exception or tries to return a
//                            value of any kind it will be ignored.
//     finalize: function() - Called if the chunking of the response is aborted for
//                            any reason. If this function throws an exception or
//                            tries to return a value of any kind it will be ignored.
//                            (Note: an object's finalize function is automatically
//                            called by SpiderMonkey on GC of the object. The reason
//                            it is NOT called if the response is sent successfully
//                            is because SG_uridispatch.c sets the response object's
//                            finalize member to null when it calls onFinish. It
//                            sets it to null right *before* calling onFinish, so if
//                            you wanted the finalize function to be called even
//                            after onFinish, it would technically still be possible
//                            by setting it again in your onFinish function.)
//
//     ...
//     And add your own members for state information needed by subsequent callbacks.

var routes = ({});

var routes_print_friendly = [];

//
// registerRoutes() is what registers mappings that match a url pattern to a set of
// handlers for the request.
//
// If a route already exists for a given url, the old route will be overridden.
//

var registerRoutes = function (new_routes)
{
    for (var path in new_routes)
    {
        var route = new_routes[path];

        if (serverConf.enable_diagnostics)
        {
            for (var verb in route)
            {
                routes_print_friendly.push({"verb": verb, "path": path});
            }
        }

        route.pathParts = trimSlashes(path).split("/");

        var pRoute = routes;
        var suffix = undefined;
        for (var i = 0; i < route.pathParts.length; ++i)
        {
            var lastPart = (i === route.pathParts.length - 1);

            var part = route.pathParts[i];

            if (lastPart)
            {
                var dotPos = part.lastIndexOf(".");
                if (dotPos !== -1)
                {
                    suffix = part.slice(dotPos + 1);
                    part = part.slice(0, dotPos);
                }
            }

            if (part[0] === "<")
            {
                route.pathParts[i] = part.slice(1, -1);
                part = "*";
            }

            if (!pRoute[part])
                pRoute[part] = ({});
            pRoute = pRoute[part];
        }

        if (!pRoute[null])
            pRoute[null] = ({});
        pRoute = pRoute[null];

        if (!pRoute[suffix])
        {
            pRoute[suffix] = route;
        }
        else
        {
            for (var verb in route)
            {
                pRoute[suffix][verb] = route[verb];
            }
        }
    }
};

//
// routeInject allows a module to add a new member variable into existing routes.
// It takes three parameters: A key, a value, and a list of paths.
// The key value pair will be added to the route(s) for each path in the list. If
// a route does not exist for a given path, routeInject will ignore that path.
// If the route exists and already has a value for the key, routeInject will not
// change the existing value. Each path should be a string starting with a slash,
// optionally with an http verb before that (without any spaces or separators).
//

var routeInject = function(key, value, paths) {
	for (var iPath in paths) {
		var path = paths[iPath];
		var verb = path.slice(0,path.indexOf("/"));
		path = path.slice(path.indexOf("/"));
		
		var suffix = undefined;
		if (path.lastIndexOf(".")>path.lastIndexOf("/")) {
			suffix = path.slice(path.lastIndexOf(".")+1);
			path = path.slice(0, path.lastIndexOf("."));
		}
		
		pathParts = trimSlashes(path).split("/");
		var pRoute = routes;
		for (var i=0; pRoute && i<pathParts.length; ++i) {
			var part = pathParts[i];
			if (part[0]==="<") {part="*";}
			pRoute = pRoute[part];
		}
		
		if (pRoute && pRoute[null] && pRoute[null][suffix]) {
			pRoute = pRoute[null][suffix];
			if (verb!=="") {
				routeInject1(key, value, pRoute[verb]);
			}
			else {
				for (verb in pRoute) {
					routeInject1(key, value, pRoute[verb]);
				}
			}
		}
	}
};

// helper function for routeInject
var routeInject1 = function(key, value, route) {
	if (route) {
		if (route[key]===undefined) {
			route[key] = value;
		}
	}
};


// onAuth is a simple mechanism by which to let a module perform authorization on all
// urls, including those in the core. Only one module may define onAuth on a given
// server. It should be a function receiving two parameters, the route object and the
// request object, and should return true or false (true meaning authorized).

var onAuth = null;


//
// dispatch() is the function that is called for every incoming request to the server
//

var dispatch = function (request)
{
    //
    // First add some more members to the request object that the C layer didn't cover.
    //

    // Make sure headers member exists
    request.headers = request.headers || {};

    // Link root
    // (Note: If the server is behind a proxy we need to use the X-Forwarded-Host
    // header to determine the correct linkRoot).
    request.scheme = request.scheme || serverConf.scheme || "http:";
    if (request.scheme.charAt(request.scheme.length - 1) != ":")
        request.scheme += ":";

    request.hostname = request.headers["X-Forwarded-Host"] || request.headers["Host"] || request.headers["host"] || serverConf.hostname;

    if (request.hostname)
        request.linkRoot = request.scheme + "//" + request.hostname + serverConf.application_root;
    else
        request.linkRoot = serverConf.application_root;

    // HTTP Cookie object
    var cookieObj = {};
    if (request.headers.Cookie)
    {
        var pairs = request.headers.Cookie.split(";");
        for (var i = 0; i < pairs.length; i++)
        {
            var parts = pairs[i].split("=");
            if (parts.length == 2)
            {
                var key = parts[0].trim();
                var val = parts[1].trim();
                cookieObj[key] = val;
            }
        }
    }
    request.cookies = cookieObj;

    // Destructor
    request.close = function ()
    {
        if (this.repo)
            this.repo.close();
    };

    //
    // Next we'll find a route that matches the uri.
    //

    var pathParts = trimSlashes(request.uri).split("/");
    var suffix = undefined;

    var lastPart = pathParts[pathParts.length - 1];
    var dotPos = lastPart.lastIndexOf(".");
    if (dotPos !== -1)
    {
        suffix = lastPart.slice(dotPos + 1);
        pathParts[pathParts.length - 1] = lastPart.slice(0, dotPos);
    }

    var substitutions = [];
    var pRoute = routes;

    for (var i = 0; i < pathParts.length; ++i)
    {
        if (pRoute[pathParts[i]])
        {
            pRoute = pRoute[pathParts[i]];
        }
        else if (pRoute["*"])
        {
            substitutions.push(i);

            pRoute = pRoute["*"];
        }
        else
            return notFoundResponse(request);
    }

    if (pRoute[null])
        pRoute = pRoute[null];
    else
        return notFoundResponse(request);

    var pNext = pRoute[suffix];
    if (!pNext)
    {
        if (pRoute["$"])
        {
            // A ".$" extension means that the route wants the file extension to be included in the last "pathPart" token.
            if (suffix !== undefined)
                pathParts[pathParts.length - 1] = pathParts[pathParts.length - 1] + "." + suffix;
            pNext = pRoute["$"];
        }
        else
        {
            pNext = pRoute["html"] || pRoute["json"] || pRoute[firstKeyIn(pRoute)];
        }
    }
    pRoute = pNext;

    if (!pRoute)
        return notFoundResponse(request);

    var pathPartNames = pRoute.pathParts;

    if (!pRoute[request.requestMethod])
    {
        var allow = "";
        for (var allowedMethod in pRoute)
        {
            if (allowedMethod !== "pathParts")
                allow += allowedMethod + " ";
            if (allowedMethod === "GET")
                allow += "HEAD ";
        }
        return methodNotAllowedResponse(request, null, allow.rtrim(" "));
    }

    request.inactiveUser = false;
    request.vvuser = request.headers.From || null;

    pRoute = pRoute[request.requestMethod];

    // Create a member in the request for each fill-in-the-blank from the path that we matched it to
    for (i = 0; i < substitutions.length; ++i)
    {
        request[pathPartNames[substitutions[i]]] = pathParts[substitutions[i]];

        if (pathPartNames[substitutions[i]] == "repoName" && pRoute.noRepo !== true)
        {
            try
            {
                request["repo"] = sg.open_repo(request["repoName"]);                
            }
            catch (ex)
            {
                if (ex.message.indexOf("Error 10 (sglib)") >= 0 || /* SG_ERR_NOTAREPOSITORY */
                    ex.message.indexOf("Error 291 (sglib)") >= 0)  /* SG_ERR_REPO_UNAVAILABLE */
                {
                    if ((request.requestMethod == "GET") &&
						(!request.uri.match(/\.json$/)) &&
						(!request.uri.match(/\.xml$/)))
                        return (redirectResponse(request, "/", "The repository '" + request["repoName"] + "' does not exist on this server or is not available."));
                    else
                        return notFoundResponse(request, "The repository '" + request["repoName"] + "' does not exist on this server or is not available.");
                }                
                else
                {
                    return response_for_exception_caught_by_dispatcher(ex, "sg.open_repo", request);
                }
            }

            try
            {
                // check for an inactive user.
                if (request.vvuser)
                {
                    var udb = new zingdb(request['repo'], sg.dagnum.USERS);
                    var rec = udb.get_record('user', request.vvuser, ['*']);

                    if (pRoute.requireUser && ((!rec) || rec.inactive))
                    {
                        var msg = null;

                        if (rec)
                            msg = "Unauthorized: User " + rec.name + " is inactive.";
                        else
                            msg = "No matching user found.";

                        return (unauthorizedResponse(request, msg));
                    }
                }
            }
            catch (ex)
            {
                return response_for_exception_caught_by_dispatcher(ex, "user check", request);
            }
        }
    }

    // Save the format with the route so we can behave differently for different content types
    pRoute.format = suffix;

    // We now have the route. See if we need to reject the request it based on readonly server.
    if (serverConf.readonly)
    {
        if (request.requestMethod === "GET" && pRoute.availableOnReadonlyServer === false)
            return notFoundResponse(request, "Resource not available when server is in readonly mode.");
        if (request.requestMethod !== "GET" && pRoute.availableOnReadonlyServer !== true)
            return methodNotAllowedResponse(request, "Operation not allowed when server is in readonly mode.");
    }

    if (request.inactiveUser)
        if (pRoute.availableToInactiveUser === false)
            return notFoundResponse(request, "Resource not available to inactive users.");

    if (pRoute.requireUser && !request.vvuser)
        return unauthorizedResponse(request);

    if (pRoute.enableSessions)
        vvSession.start(request); // Initialize vvSession for this client.

    //
    // We've mapped the uri (+ request method) to a route, which should provide
    // us with all the callback functions we need to construct a repsonse.
    //
    // Before we call into the route's functions, though, we'll call onAuth (if
    // provided by a module) to authorize the request.

    if (onAuth)
    {
        try
        {
            sg.server.request_profiler_start(sg.server.AUTH);
            var ok = onAuth(pRoute, request);
            sg.server.request_profiler_stop();
            if (ok === false)
            {
                return unauthorizedResponse(request);
            }
            else if (ok !== true)
            {
                return response_for_exception_caught_by_dispatcher("onAuth did not return a true/false value", "dispatch", request);
            }
        }
        catch (ex)
        {
            sg.server.request_profiler_stop();
            return response_for_exception_caught_by_dispatcher(ex, "onAuth", request);
        }
    }

    //
    // Ok, now we'll call the route's onDispatch function, if it has one,
    // and see if it can produce a response based on the http headers alone.
    //

    if (pRoute.onDispatch != undefined)
    {
        try
        {
            sg.server.request_profiler_start(sg.server.ONDISPATCH);
            var response = pRoute.onDispatch(request);

            if (response !== undefined && response !== null)
            {
                verify_response_object(response);
                sg.server.request_profiler_stop();
                return response;
            }
        }
        catch (ex)
        {
            sg.server.request_profiler_stop();
            return response_for_exception_caught_by_dispatcher(ex, "onDispatch callback", request);
        }
    }

    //
    // A response has not yet been generated based on the request headers.
    // We now construct a request-handler object with the route's callback
    // functions to handle the rest of the request. Then we'll be able to
    // form a response after receiving the message body.
    //

    var requestHandler = { request: request };
    if (pRoute.onJsonReceived)
    {
        requestHandler.onJsonReceived_real = pRoute.onJsonReceived;
        requestHandler.onJsonReceived = function (request, data)
        {
            try
            {
                sg.server.request_profiler_start(sg.server.ONPOSTRECEIVED);
                var response = this.onJsonReceived_real(request, data);
                verify_response_object(response);
                sg.server.request_profiler_stop();
                return response;
            }
            catch (ex)
            {
                sg.server.request_profiler_stop();
                return response_for_exception_caught_by_dispatcher(ex, "onJsonReceived callback", request);
            }
        };
        requestHandler.onJsonReceived_ = true;
    }
    if (pRoute.onFileReceived)
    {
        requestHandler.onFileReceived_real = pRoute.onFileReceived;
        requestHandler.onFileReceived = function (request, file_path)
        {
            try
            {
                sg.server.request_profiler_start(sg.server.ONPOSTRECEIVED);
                var response = this.onFileReceived_real(request, file_path);
                verify_response_object(response);
                sg.server.request_profiler_stop();
                return response;
            }
            catch (ex)
            {
                sg.server.request_profiler_stop();
                return response_for_exception_caught_by_dispatcher(ex, "onFileReceived callback", request);
            }
        };
        requestHandler.onFileReceived_ = true;
    }
    if (pRoute.onMimeFileReceived)
    {
        requestHandler.onMimeFileReceived_real = pRoute.onMimeFileReceived;
        requestHandler.onMimeFileReceived = function (request, file_path, filename, mimetype)
        {
            try
            {
                sg.server.request_profiler_start(sg.server.ONPOSTRECEIVED);
                var response = this.onMimeFileReceived_real(request, file_path, filename, mimetype);
                verify_response_object(response);
                sg.server.request_profiler_stop();
                return response;
            }
            catch (ex)
            {
                sg.server.request_profiler_stop();
                return response_for_exception_caught_by_dispatcher(ex, "onMimeFileReceived callback", request);
            }
        };
        requestHandler.onMimeFileReceived_ = true;
    }
    if (pRoute.onRequestAborted)
    {
        requestHandler.onRequestAborted = pRoute.onRequestAborted;
    }

    return requestHandler;
};

// Helper function for dispatch(). Simply throws and exception if an object is not a valid "repsonse object".
var verify_response_object = function(obj) {
	// undefined and null are considered valid:
	// - When returned by an onDispatch() callback, this means the actual response will be generated later.
	// - When returned by an onReceived() callback, this implies the request was processed with no errors, and
	//   the C layer can just return a "200 OK" response with no message body.
	if(obj!==undefined && obj!==null)
	{
		if(!obj.statusCode)
			throw "Callback returned an object without a status code";
		if(!(obj.headers["Content-Length"]>=0))
			throw "Callback returned an object with missing or invalid Content-Length.";
	}
};

// Helper function for dispatch(). Creates an error response based on an exception that was caught.
var response_for_exception_caught_by_dispatcher = function (ex, functionName, request)
{    
    var is_response = false;
    try
    {
        verify_response_object(ex);
        is_response = true;
    }
    catch (ex2)
    {
    }

    if (is_response)
    {
        // An actual response object was thrown. Just return it as THE response.
        return ex;
    }
    else
    {
		// Is this a common error we want to handle specially?
		if (ex.message)
		{
			var repoName = " ";
			if (request["repoName"])
				repoName = " '" + request["repoName"] + "' ";

			if (ex.message.indexOf("Error 208 (sglib)") >= 0) /* SG_ERR_OLD_AUDITS */
			{
				return errorResponse(STATUS_CODE__INTERNAL_SERVER_ERROR, request, "The" + repoName + "repository has an old format. Use 'vv upgrade' to upgrade it.");
			}
			if (ex.message.indexOf("Error 275 (sglib)") >= 0) /* SG_ERR_NEED_REINDEX */
			{
				return errorResponse(STATUS_CODE__INTERNAL_SERVER_ERROR, request, "The" + repoName + "repository has indexes in an old format. Use 'vv reindex' to update them.");
			}
		}

        // Construct a generic response for an unhandled exception.
        sg.log('urgent', functionName + " threw exception during request to " + request.uri + ":\n" + ex.toString());

        if (serverConf.enable_diagnostics)
            return errorResponse(STATUS_CODE__INTERNAL_SERVER_ERROR, request, ex.toString());
        else
            return errorResponse(STATUS_CODE__INTERNAL_SERVER_ERROR, request, "Internal Server Error. Details about this error have been logged to the server's log file.");
    }
};

// This function is called from C code when the response is all ready to be sent.
// All it does is save the current session state, and insert the correct headers
// into the response for the session.
var onBeforeSendResponse = function(response) {
	// (The C code ignores all output and errors generated by this function,
	// so we'll wrap in a try/catch and log any exception.)
	var session = null;
	try
	{
		if( response && (response.statusCode==STATUS_CODE__OK) )
		{
			session = vvSession.get();
			if (session)
			{
				vvSession.setCookies(response, session);
				vvSession.save(session);
			}
			formatCookies(response);
		}
	}
	catch(ex)
	{
		sg.log('urgent', "Exception caught in onBeforeSendResponse: " + ex.toString());
	}
};

// This function is called from C code when it is finished processing a request,
// no matter whether it sent a successful response, an error response, or
// completely aborted and didn't even finish receiving the request and/or
// sending a response.
var onDispatchContextNullfree = function() {
	vvSession.reset();
};

function formatCookies(response)
{
	// format cookies
	if (response.headers && response.headers['Set-Cookie'])
	{
		var toFormat = response.headers['Set-Cookie'];

		if (Array.isArray(toFormat))
		{
			var cookies = [];
			for (var i = 0; i < toFormat.length; i++)
			{
				var cookie = toFormat[i];
				if (typeof cookie == "string")
				{
					cookies.push(cookie);
				}
				else
				{
					var formatted = formatCookieObject(cookie);
					if (formatted != "")
						cookies.push(formatted);
				}
			}

			response.headers['Set-Cookie'] = cookies;
		}
		else if (typeof toFormat == "object")
		{
			response.headers['Set-Cookie'] = formatCookieObject(toFormat);
		}
	}
}

/**
 * Take a cookie object and convert it to a string that is valid to be used in a
 * Set-Cookie HTTP header
 *
 * The cookie object can be and JS object with following keys:
 *
 * name (required) -	The name of the cookie
 *
 * value (required) -	The value to be set in the cookie.  This my be a  string.
 * 						If it is not a string it will be saved in the cookie as
 * 						whatever value toString() returns for that value.
 *
 * expires (optional) -	The date at which the cookie should expire.  This can
 * 						be a Javascript Date object or a string that represents
 * 						a valid expiration date for a HTTP cookie.
 *
 * max_age (optional) -	The number of seconds in the future the cookie should
 * 						expire.
 *
 * path (optional) -	The server path that this cookie should be returned for.
 *
 * domain (optional) -	The domain this cookie should be valid for.
 *
 * version (optional) - The version number for the cookie.
 *
 * secure (optional) - 	A boolean that tells the client to only send to cookie
 * 						back to the server when using a secure connection.
 *
 * httponly (optional) - A boolean that tells the client that the cookie should
 * 						 only be used over http connections and should not be
 * 						 visible to client side scripts.
 *
 * = Cookie Lifespan =
 * It should be noted that if both expires and max_age are excluded the cookie
 * will be considered a session cookie and will expire when the user closes their
 * browser window.
 *
 **/
function formatCookieObject(obj)
{
	// If the cookie object is a string, assume the caller has already formatted
	// the cookie string properly
	if (typeof obj == "string")
		return obj;

	//  Name and Value are required
	if (!obj.name || !obj.value)
		return "";

	var parts = [];
	var base = obj.name + "=" + obj.value;
	parts.push(base);

	if (obj.expires)
	{
		if (obj.expires.getDate)
		{
			parts.push("Expires=" + obj.expires.strftime("%a, %d-%b-%Y %T %Z"));
		}
		else
		{
			parts.push("Expires=" + obj.expires);

		}
	}

	if (obj.max_age)
	{
		parts.push("Max-Age=" + obj.max_age);
	}

	if (obj.path)
	{
		parts.push("path=" + obj.path);
	}

	if (obj.domain)
	{
		parts.push("domain=" + obj.domain);
	}

	if (obj.version)
	{
		parts.push("Version=" + obj.version);
	}

	if (obj.secure)
	{
		parts.push("Secure");
	}

	if (obj.httponly)
	{
		parts.push("httponly");
	}

	return parts.join("; ");
}
