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

// Config vars
var sessionLifetimeSeconds = 60 * 60 * 3; // 3 hours
var sessionCleanupIntervalSeconds = 600;
var sessionVersion = 1;

/**
 * vvSession - Session State methods
 *
 * This library provides a simple file based mechanism for persisting data between
 * HTTP requests.  
 *
 * = Usage =
 * Sessions are not automatically created on each request.  In order to request
 * that a session object be created, you must set enableSessions to true inside 
 * your route definition:
 * 
 * <pre>
 *	"/test/cookies/session": {
 *		"GET": {
 *			enableSessions: true,
 *			onDispatch: function (request)
 *			{
 *				var session = vvSession.get();
 *				return OK;
 *			}
 *		}
 *	},
 * </pre>
 *
 * Once you have a session you can store data in the session by adding it like 
 * you would for any other JavaScript object.
 *
 * <pre>
 * 		var session = vvSession.get();
 * 		var data = { key: "value", foo: "bar" };
 * 		session["store_me"] = data;
 * 		session.bar = "some value";
 * 		delete session.bar;
 * </pre>
 *
 * The session will serialize itself to disk at the end of the request.
 **/

var vvSession = (function ()
{
    var _inited = false;
    var options = {};
    var rootInstance = null;
    var lastSessionCleanup = 0;

    var init = function ()
    {
        var sessionDir = serverConf.session_storage;

        if (sessionDir)
        {
            if (!sessionDir.match(/[\\\/]$/))
                sessionDir += "/";
        }
        else
        {
            sessionDir = sg.server_state_dir();

            if (!sessionDir.match(/[\\\/]$/))
                sessionDir += "/";

            sessionDir += "vvsession/";
        }

        if (!sg.fs.exists(sessionDir))
        {
            sg.fs.mkdir(sessionDir);
            sg.fs.chmod(sessionDir, 0700);
        }

        options["sessionDir"] = sessionDir;
    };

    var start = function (request)
    {
        if (!_inited)
            init();

		sg.server.request_profiler_start(sg.server.SESSION_READ);
		try{
			if (!rootInstance || !request.cookies || (request.cookies['vvSession'] == undefined))
			{
				rootInstance = null;
				rootInstance = new vvSessionInstance(request);
			}
			else
			{
				// If the root instance already exists, check that the hash
				// matches what we have in our session cookie, just in case the context
				// wasn't properly cleaned up at the end of the last request
				
				if (rootInstance.hash != request.cookies['vvSession'])
				{
					sg.log("debug", "Session hash (" + rootInstance.hash + ") didn't match session cookie (" + request.cookies['vvSession'] + "). Reloading from file.");
					rootInstance = null;
					rootInstance = new vvSessionInstance(request);
				}	
			}

			return rootInstance;
		}
		finally{
			sg.server.request_profiler_stop();
		}
    };

    var get = function ()
    {
        if (!rootInstance)
            return null;

        return rootInstance;
    };

    var setCookies = function (response, instance)
    {
        if (!response.headers)
            response.headers = {};

        var session_cookie = {
            name: "vvSession",
            value: instance.hash,
            max_age: sessionLifetimeSeconds,
            version: sessionVersion,
            path: "/"
        };

        if (!response.headers["Set-Cookie"])
        {
            response.headers["Set-Cookie"] = [];
        }
        else
        {
            if (typeof response.headers["Set-Cookie"] != "array")
            {
                var existing = response.headers["Set-Cookie"];
                response.headers["Set-Cookie"] = [existing];
            }
        }

        response.headers["Set-Cookie"].push(session_cookie);
    };

    var save = function (instance)
    {
        sg.server.request_profiler_start(sg.server.SESSION_WRITE);
        try{
			var json = sg.to_json(instance);
			var path = sessionPath(instance);

			removeOldSessions();

			if (!json)
				return;
	
			sg.mutex.lock(instance.hash);
			try
			{
				sg.file.write(path, json, 0600);
			}
			finally
			{
				sg.mutex.unlock(instance.hash);
			}
		}
		finally{
			sg.server.request_profiler_stop();
		}
    };

    var reset = function ()
    {
        rootInstance = null;
    };

    var removeOldSessions = function ()
    {
        var now = new Date();
        if (now - lastSessionCleanup < sessionCleanupIntervalSeconds * 1000)
        {
            //sg.log("verbose", "Sessions were recently cleaned up. Skipping.");
            return;
        }
        lastSessionCleanup = now;

        if (!sg.fs.exists(options.sessionDir))
            return;

        //sg.log("verbose", "Cleaning up old sessions.");
        
        var ts_now = now.getTime();

		sg.server.request_profiler_start(sg.server.SESSION_CLEANUP);
        sg.mutex.lock("SESSION_CLEANUP");
        try
        {
            var files = sg.fs.readdir(options.sessionDir);

            for (var i = 0; i < files.length; ++i)
            {
                var file = files[i];

                // Remove session files that are older than the session lifetime
                if ((ts_now - (sessionLifetimeSeconds * 1000)) > file.modtime.getTime())
                {
                    var path = options.sessionDir + file.name;

                    try
                    {
                        sg.fs.remove(path);
                        //sg.log("verbose", "Succesfully removed expired session file: " + path);
                    }
                    catch(ex)
                    {
                        if (sg.fs.exists(path))
                            sg.log("urgent", "Error removing session file: " + path + " " + ex);
                    }
                }
            }
        }
        catch (ex)
        {
            sg.log("urgent", "Error cleaning up old sessions: "+ ex);
        }
        finally
        {
            sg.mutex.unlock("SESSION_CLEANUP");
            sg.server.request_profiler_stop();
        }
    };

    var sessionPath = function (instance)
    {
        return options.sessionDir + sessionFileName(instance);
    };

    var sessionFileName = function (instance)
    {
        return instance.hash;
    };

    var vvSessionInstance = function (request)
    {
        var _hash = null;

        var init = function (instance)
        {
            if (request.cookies['vvSession'])
            {
                _hash = request.cookies['vvSession'];
                instance.hash = _hash;
                load(instance);
            }
            else
            {
                _hash = sessionHash();
                instance.hash = _hash;
            }

            return instance;
        };

        var load = function (instance)
        {
            var path = sessionPath(instance);

            if (!sg.fs.exists(path))
            {
                return;
            }

            sg.mutex.lock(instance.hash);
            var json = null;
            try
            {
                json = sg.file.read(path);
            }
            finally
            {
                sg.mutex.unlock(instance.hash);
            }

            if (json === null)
                return;

            var failed = false;
            try
            {
                var obj = sg.from_json(json);
            }
            catch (e)
            {
                failed = true;
            }

            if (failed)
                return;

            for (var key in obj)
            {
                if (key == "hash")
                    continue;

                instance[key] = obj[key];
            }
        };

        var sessionHash = function ()
        {
            if (_hash)
                return _hash;

            var t = (new Date()).getTime();
            var r = Math.random();
            var key = "" + t + r;
            var hash = sg.hash(key);
            return "vv_" + hash;
        };

        return init(this);
    };

    // export the public API
    return {
        start: start,
        get: get,
        setCookies: setCookies,
        save: save,
        reset: reset
    };
})();
