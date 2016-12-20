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
    "/local/file-download": {
        "GET": {
            onDispatch: function (request)
            {
                var p = vv.getQueryParam("path", request.queryString);
                var top = sg.fs.getcwd_top();
                var path = top + "/" + p;
                var filename = vv.getQueryParam("name", request.queryString);
                return fileResponse(path, false, null, true, filename);
            }
        }
    },
    "/local/file": {
        "GET": {
            onDispatch: function (request)
            {
                var p = vv.getQueryParam("path", request.queryString);
                var top = sg.fs.getcwd_top();
                var path = top + "/" + p;
                var filename = vv.getQueryParam("name", request.queryString);
                return fileResponse(path, false, null, false, filename);
            }
        }
    },

    "/local/status.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cwd = sg.fs.getcwd_top();
		// TODO 2011/11/11 sg.wc.status() has a bunch of options now.
		// TODO            think about whether we want to expose them.
                var res = sg.wc.status();
                return jsonResponse({ "status": res, "cwd": cwd });
            }
        }

    },
	"/local/status.html": {
		"GET": {
			onDispatch: function (request)
			{
				var vvt = new vvUiTemplateResponse(
							request, "core", "Status"
						);

				return (vvt);
			}
		}
	},

    "/local/mstatus.json": {
        "GET": {
            onDispatch: function (request)
            {
                var cwd = sg.fs.getcwd_top();
                var res = sg.wc.mstatus();
                return jsonResponse({ "status": res, "cwd": cwd });
            }
        }

    },

    "/local/fs.html": {
        "GET": {
            onDispatch: function (request)
            {
                var vvt = new vvUiTemplateResponse(request, "core", "Working Copy", null, "browsefs.html");
                return (vvt);
            }
        }
    },
    "/local/fs.json": {
        "GET": {
            onDispatch: function (request)
            {
                var path = vv.getQueryParam("path", request.queryString);
                var cwd = sg.fs.getcwd_top();

                var fullPath = cwd;
                if (path)
                {
                    path = path.ltrim("/");
                    fullPath = cwd + "/" + path;
                }
                if (!sg.fs.exists(fullPath))
                {
                    return (notFoundResponse());
                }
                var direntries = sg.fs.readdir(fullPath);

                return jsonResponse({ "files": direntries, "cwd": cwd });
            }
        }
    }
});
