/*
Copyright 2012-2013 SourceGear, LLC

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

load("../js_test_lib/vscript_test_lib.js");

load(sg.local_settings().machine.server.files+"/dispatch.js");
serverConf = sg.local_settings().machine.server; // Normally set by C code calling dispatch's init().

function st_ssjs_module_auth() {
	
	this.setUp = function()
	{
		onAuth = function(route, request) {
			return (route.open_for_testing===true);
		}
		registerRoutes({
			"/module_auth/open": {
				"GET": {
					open_for_testing: true,
					onDispatch: function(request) {return OK;}
				},
				"POST": {
					open_for_testing: true,
					onDispatch: function (request) {return badRequestResponse(request, "Returning 400 for testing purposes, hooray!");},
				}
			},
			"/module_auth/closed": {
				"GET": {
					onDispatch: function(request) {return OK;}
				},
				"POST": {
					onDispatch: function (request) {return badRequestResponse(request, "Returning 400 for testing purposes, hooray!");},
				}
			},
			"/pseudocore/open-get.txt": {
				"GET": {
					onDispatch: function(request) {return OK;}
				},
				"POST": {
					onDispatch: function (request) {return badRequestResponse(request, "Returning 400 for testing purposes, hooray!");},
				}
			},
			"/pseudocore/open-all.txt": {
				"GET": {
					onDispatch: function(request) {return OK;}
				},
				"POST": {
					onDispatch: function (request) {return badRequestResponse(request, "Returning 400 for testing purposes, hooray!");},
				}
			},
			"/pseudocore/open--no-file-extension": {
				"GET": {
					onDispatch: function(request) {return OK;}
				}
			},
		});
		routeInject("open_for_testing", true, [
			"GET/pseudocore/open-get.txt",
			"/pseudocore/open-all.txt",
			"/pseudocore/open--no-file-extension"]);
	};
	
	this.doTest = function(verb, uri, expectedStatus){
		var o = dispatch({requestMethod:verb, uri:uri});
		testlib.equal(expectedStatus, o.statusCode, verb+" "+uri);
		try{o.finalize();}catch(ex){} // Not sure why this wasn't getting called automatically (at least not before our leak-checking...)...
	};
	
	this.testNormalRoutes = function() {
		this.doTest("GET", "/module_auth/open", STATUS_CODE__OK);
		this.doTest("POST", "/module_auth/open", STATUS_CODE__BAD_REQUEST);
		this.doTest("GET", "/module_auth/closed", STATUS_CODE__UNAUTHORIZED);
		this.doTest("POST", "/module_auth/closed", STATUS_CODE__UNAUTHORIZED);
	};
	
	this.testInjectedRoutes = function() {
		this.doTest("GET", "/pseudocore/open-get.txt", STATUS_CODE__OK);
		this.doTest("POST", "/pseudocore/open-get.txt", STATUS_CODE__UNAUTHORIZED);
		this.doTest("GET", "/pseudocore/open-all.txt", STATUS_CODE__OK);
		this.doTest("POST", "/pseudocore/open-all.txt", STATUS_CODE__BAD_REQUEST);
		this.doTest("GET", "/pseudocore/open--no-file-extension", STATUS_CODE__OK);
	};
}
