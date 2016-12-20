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

if (serverConf.enable_diagnostics) registerRoutes({
    "/test/remote-shutdown": {
        "POST": {
            availableOnReadonlyServer: true,
            onJsonReceived: function (request, data)
            {
                if (data.test === true)
                    sg.server.debug_shutdown();
                return OK;
            }
        }
    },
    "/test/responses/template-test.html": {
        "GET": {
            onDispatch: function (request)
            {
                var vvt = new vvUiTemplateResponse(request, "core", "test", null, "test.html");

                return (vvt);
            }
        }
    },
    "/test/responses/ok": {
        "GET": {
            onDispatch: function (request)
            {
                return OK;
            }
        },
        "POST": {
            onDispatch: function (request)
            {
                request.my_response_to_return = OK;
            },
            onFileReceived: function (request, file_path)
            {
                return request.my_response_to_return;
            }
        }
    },
    "/test/responses/ok/thrown": {
        "GET": {
            onDispatch: function (request)
            {
                throw OK;
            }
        },
        "POST": {
            onDispatch: function (request)
            {
                request.my_response_to_return = OK;
            },
            onFileReceived: function (request, file_path)
            {
                throw request.my_response_to_return;
            }
        }
    },
    "/test/responses/implicit-200-ok": {
        "POST": {
            onFileReceived: function (request, file_path)
            {
            }
        }
    },
    "/test/responses/text": {
        "GET": {
            onDispatch: function (request)
            {
                return textResponse("Test");
            }
        }
    },
    "/test/responses/text/js-exception-in-onchunk": {
        "GET": {
            onDispatch: function (request)
            {
                var response = {};
                response.statusCode = STATUS_CODE__OK;
                response.headers = ({
                    "Content-Length": 12,
                    "Content-Type": CONTENT_TYPE__TEXT
                });
                response.numChunksSent = 0;
                response.onChunk = function ()
                {
                    if (response.numChunksSent == 0)
                    {
                        ++response.numChunksSent;
                        return sg.to_sz("12345")
                    }
                    else
                    {
                        throw "Exception thrown by \"/test/responses/text/js-exception-in-onchunk\" route as part of test.";
                    }
                };
                response.finalize = function ()
                {
                    sg.log("finalize() called on response sent for \"/test/responses/text/js-exception-in-onchunk\"");
                };
                response.onFinish = function ()
                {
                    sg.log("onFinish() called on response sent for \"/test/responses/text/js-exception-in-onchunk\"");
                };
                return response;
            }
        }
    },
    "/test/responses/json": {
        "GET": {
            onDispatch: function (request)
            {
                return jsonResponse({ test: true });
            }
        }
    },
    "/test/responses/bad_request": {
        "GET": {
            onDispatch: function (request)
            {
                return badRequestResponse();
            }
        }
    },
    "/test/responses/bad_request/thrown": {
        "GET": {
            onDispatch: function (request)
            {
                throw badRequestResponse();
            }
        }
    },
    "/test/responses/precondition_fail": {
        "GET": {
            onDispatch: function (request)
            {
                return preconditionFailedResponse(null, "fail");
            }
        }
    },
    "/test/responses/method_not_allowed": {
        "GET": {
            onDispatch: function (request)
            {
                return methodNotAllowedResponse();
            }
        }
    },
    "/test/responses/method_not_allowed/except_on_post": {
        "POST": {
            onFileReceived: function (request, file_path)
            {
                return OK;
            }
        }
    },
    "/test/responses/not_found": {
        "GET": {
            onDispatch: function (request)
            {
                return notFoundResponse();
            }
        }
    },
    "/test/responses/unauthorized": {
        "GET": {
            onDispatch: function (request)
            {
                return unauthorizedResponse();
            }
        }
    },
    "/test/responses/500/js-exception-in-ondispatch.html": {
        "GET": {
            onDispatch: function (request)
            {
                throw "Exception thrown by \"/test/responses/500/js-exception-in-ondispatch.html\" route as part of test.";
            }
        }
    },
    "/test/responses/500/js-exception-in-ondispatch.json": {
        "GET": {
            onDispatch: function (request)
            {
                throw "Exception thrown by \"/test/responses/500/js-exception-in-ondispatch.json\" route as part of test.";
            }
        }
    },
    "/test/responses/500/js-exception-in-onjsonreceived": {
        "POST": {
            onJsonReceived: function (request, data)
            {
                throw "Exception thrown by \"/test/responses/500/js-exception-in-onjsonreceived\" route as part of test.";
            }
        }
    },
    "/test/responses/500/js-exception-in-onfilereceived": {
        "POST": {
            onFileReceived: function (request, file_path)
            {
                throw "Exception thrown by \"/test/responses/500/js-exception-in-onfilereceived\" route as part of test.";
            }
        }
    },
    "/test/responses/500/js-exception-in-onmimefilereceived": {
        "POST": {
            onMimeFileReceived: function (request, path, filename, mimetype)
            {
                throw "Exception thrown by \"/test/responses/500/js-exception-in-onmimefilereceived\" route as part of test.";
            }
        }
    },
    "/test/responses/500/invalid-response/undefined": {
        "GET": {
            onDispatch: function (request)
            {
            }
        }
    },
    "/test/responses/500/invalid-response/null": {
        "GET": {
            onDispatch: function (request)
            {
                return null;
            }
        },
        "POST": {
            onFileReceived: function (request, file_path)
            {
                return null;
            }
        }
    },
    "/test/responses/500/invalid-response/object": {
        "GET": {
            onDispatch: function (request)
            {
                return { "S-t-a-t-u-s C-o-d-e": STATUS_CODE__BAD_REQUEST };
            }
        },
        "POST": {
            onFileReceived: function (request, file_path)
            {
                return { "S-t-a-t-u-s C-o-d-e": STATUS_CODE__BAD_REQUEST };
            }
        }
    },
    "/test/responses/500/invalid-response/int": {
        "GET": {
            onDispatch: function (request)
            {
                return 400;
            }
        },
        "POST": {
            onFileReceived: function (request, file_path)
            {
                return 400;
            }
        }
    },
    "/test/responses/500/invalid-response/string": {
        "GET": {
            onDispatch: function (request)
            {
                return "400 Bad Request";
            }
        },
        "POST": {
            onFileReceived: function (request, file_path)
            {
                return "400 Bad Request";
            }
        }
    },
    "/test/cookies/basic": {
        "GET": {
            enableSessions: true,
            onDispatch: function (request)
            {
                var resp = OK;

                resp.headers["Set-Cookie"] = {
                    name: "testCookie",
                    value: "testValue",
                    max_age: 10
                };

                return resp;
            }
        }
    },
    "/test/cookies/session": {
        "GET": {
            enableSessions: true,
            onDispatch: function (request)
            {
                return OK;
            }
        }
    },
    "/test/headers/x-powered-by": {
        "GET": {
            onDispatch: function (request)
            {
                var resp = OK;
                resp.headers["X-Powered-By"] = "veracity";
                return resp;
            }
        }
    },
    "/test/session/persist/<key>": {
        "POST": {
            enableSessions: true,
            onJsonReceived: function (request, data)
            {
                var session = vvSession.get();
                var key = request.key;
                session[key] = data;
                return OK;
            }
        },
        "GET": {
            enableSessions: true,
            onDispatch: function (request)
            {
                var session = vvSession.get();
                var key = request.key;
                var data = session[key];

                if (data)
                    return jsonResponse(data);
                else
                    return notFoundResponse();
            }
        }
    },
    "/test/readonly/default.txt": {
        "GET": {
            onDispatch: function (request)
            {
                return textResponse("Resource always available.");
            }
        },
        "POST": {
            onFileReceived: function (request, file_path)
            {
                return textResponse("Resource denied with 405 error when server is in readonly mode.");
            }
        }
    },
    "/test/readonly/explicitly-enabled.txt": {
        "GET": {
            availableOnReadonlyServer: true,
            onDispatch: function (request)
            {
                return textResponse("Resource always available.");
            }
        },
        "POST": {
            availableOnReadonlyServer: true,
            onFileReceived: function (request, file_path)
            {
                return textResponse("Resource always available.");
            }
        }
    },
    "/test/readonly/explicitly-disabled.txt": {
        "GET": {
            availableOnReadonlyServer: false,
            onDispatch: function (request)
            {
                return textResponse("Resource denied with 404 error when server is in readonly mode.");
            }
        },
        "POST": {
            availableOnReadonlyServer: false,
            onFileReceived: function (request, file_path)
            {
                return textResponse("Resource denied with 405 error when server is in readonly mode.");
            }
        }
    },
    "/test/routes.json": {
        "GET": {
            onDispatch: function (request)
            {
                var authRoutes = [];
                var noAuthRoutes = [];
              
                vv.each(routes_print_friendly, function (i, v)
                {
                    if ((onAuth != null) && vv.inArray(v.path, ignoreAuthPaths) >= 0)
                    {
                        noAuthRoutes.push(v);
                    }
                    else
                    {
                        authRoutes.push(v);
                    }
                });
                return jsonResponse({ auth: authRoutes, noAuth: noAuthRoutes });
            }
        }
    }
});
