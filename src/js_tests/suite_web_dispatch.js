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

load("../js_test_lib/vscript_test_lib.js");

function suite_web_dispatch() {
	
	this._testRepos = [];
	
	this.suite_setUp = function() {
		{
			var repo = sg.open_repo(repInfo.repoName);
		
			{
				var zs = new zingdb(repo, sg.dagnum.USERS);
				var urec = zs.query('user', ['*'], "name == '" + repInfo.userName + "'");

				this.userId = urec[0].recid;
			}

			repo.set_user(repInfo.userName);

			repo.close();
			
			var repos = [
				"www.veracity.com",
				"repo.json",
				"veracity"
			];
			
			for (repo in repos)
			{
				rInfo = createNewRepo(repos[repo]);
				
				this._testRepos.push(rInfo.repoName);
			}
		}
	};
	
	this.firstLineOf = function(st) {
		var t = st.replace(/[\r\n]+/, "\n");
		var lines = t.split("\n");

		if (lines.length == 0)
			return ("");

		return (lines[0]);
	};

	this.checkStatus = function(expected, actual, msg) { return testlib.equal(expected, actual, msg); }

	this.checkHeader = function(header, expected, headerBlock, msg)
	{
		var s = headerBlock.replace(/[\r\n]+/g, "\n");
		var lines = s.split("\n");
		var got = [];
		
		var prefix = "";

		if (!!msg)
			prefix = msg + ": ";
			
		var regex = new RegExp("^" + header + ":[ \t]+(.+?)[ \t]*$","i");
		
		for (var i in lines) 
		{
			var line = lines[i];
			
			if (line.indexOf(header) == 0)
			{
				
				var matches = line.match(regex);
				if (matches)
				{
					if (matches[1] == expected)
						return testlib.testResult(true);
					else
						got.push(matches[1]);
				}
			}
		}
		
		var err = "expected '" + expected + "', got '" + got.join("' and '") + "'";
		
		return (testlib.fail(err));
	};
	
	this.cookieWasSet = function(name, headerBlock, msg)
	{
		var s = headerBlock.replace(/[\r\n]+/g, "\n");
		var lines = s.split("\n");
		var got = [];
		
		var prefix = "";

		if (!!msg)
			prefix = msg + ": ";
			
		var regex = new RegExp("^Set-Cookie:[ \t]+(" + name + ")=.+[ \t]*$","i");
		
		for (var i in lines) 
		{
			var line = lines[i];
			
			var matches = line.match(regex);
			if (matches)
			{
				if (matches[1] == name)
					return testlib.testResult(true);
				else
					got.push(matches[1]);
			}
		}
		
		var err = "expected cookie name'" + name + "', got '" + got.join("' and '") + "'";
		
		return (testlib.fail(err));
	};
	
	this.getCookieValue = function(name, headerBlock)
	{
		var s = headerBlock.replace(/[\r\n]+/g, "\n");
		var lines = s.split("\n");
		
		var regex = new RegExp("^Set-Cookie:[ \t]+(" + name + ")=([^;]*).+[ \t]*$","i");
		
		for (var i in lines) 
		{
			var line = lines[i];
			
			var matches = line.match(regex);
			if (matches)
			{
				if (matches[1] == name)
					return matches[2]
			}
		}
		
		return null;
	};

	this.testOKResponse = function()
	{
		var url = this.rootUrl + "/test/responses/ok";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
	};
	this.testOKResponse_Post = function()
	{
		var url = this.rootUrl + "/test/responses/ok";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
	};
	this.testOKResponse_Thrown = function()
	{
		var url = this.rootUrl + "/test/responses/ok/thrown";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				url);
		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
	};
	this.testOKResponse_Post_Thrown = function()
	{
		var url = this.rootUrl + "/test/responses/ok/thrown";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
	};
	
	this.testImplicitOKResponse = function()
	{
		var url = this.rootUrl + "/test/responses/implicit-200-ok";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
	};
	
	this.testJSONResponse = function()
	{
		var url = this.rootUrl + "/test/responses/json";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Type", "application/json;charset=UTF-8", o.headers);
		testlib.equal('{"test":true}', o.body);
	};
	
	this.testTextResponse = function()
	{
		var url = this.rootUrl + "/test/responses/text";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("Test", o.body);
	};
	
	this.testBadRequestResponse = function()
	{
		var url = this.rootUrl + "/test/responses/bad_request";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("400 Bad Request", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("Bad request.", o.body);
	};
	this.testBadRequestResponse_Thrown = function()
	{
		var url = this.rootUrl + "/test/responses/bad_request/thrown";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("400 Bad Request", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("Bad request.", o.body);
	};
	
	this.testPreconditionFailedResponse = function()
	{
		var url = this.rootUrl + "/test/responses/precondition_fail";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url)

		this.checkStatus("412 Precondition Failed", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("fail", o.body);
	};
	
	this.testMethodNotAllowedResponse = function()
	{
		var url = this.rootUrl + "/test/responses/method_not_allowed";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("405 Method Not Allowed", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("405 Method Not Allowed", o.body);
	};
	
	this.testMethodNotAllowedResponse_ForReal = function()
	{
		var url = this.rootUrl + "/test/responses/method_not_allowed";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				url);

		this.checkStatus("405 Method Not Allowed", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("405 Method Not Allowed", o.body);
	};
	
	this.testMethodNotAllowedResponse_ForReal2 = function()
	{
		var url = this.rootUrl + "/test/responses/method_not_allowed/except_on_post";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("405 Method Not Allowed", o.status);
		this.checkHeader("Allow", "POST", o.headers);
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
	};
	
	this.testNotFoundResponse = function()
	{
		var url = this.rootUrl + "/test/responses/not_found";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("404 Not Found", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("Not found.", o.body);
	};
	this.testNotFoundResponse_ForReal = function()
	{
		var url = this.rootUrl + "/test/responses/not_found/really_really_not_found";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("404 Not Found", o.status);
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
	};
	this.testNotFoundResponse_ForReal2 = function()
	{
		var url = this.rootUrl + "/test/responses/not_found/really_really_not_found.json";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);
				

		this.checkStatus("404 Not Found", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("Not found.", o.body);
	};
	
	this.testAuthorizationRequiredResponse = function()
	{
		var url = this.rootUrl + "/test/responses/unauthorized";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("401 Authorization Required", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		testlib.equal("Authorization Required.", o.body);
	};
	
	this.test500Response_exception_onDispatch = function()
	{
		var o;

		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				this.rootUrl + "/test/responses/500/js-exception-in-ondispatch.json");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				this.rootUrl + "/test/responses/500/js-exception-in-ondispatch.html");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
	};
	this.test500Response_exception_onJsonReceived = function()
	{
		var o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "{\"test\":true}",
				this.rootUrl + "/test/responses/500/js-exception-in-onjsonreceived");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
	};
	this.test500Response_exception_onFileReceived = function()
	{
		var o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "File contents.",
				this.rootUrl + "/test/responses/500/js-exception-in-onfilereceived");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
	};
	
	this.testExceptionInOnChunk = function()
	{
		var o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				this.rootUrl + "/test/responses/text/js-exception-in-onchunk");
		if (o.exit_status==0) {
			this.checkStatus("500 Internal Server Error", o.status);
		}
		else {
			testlib.equal(18, o.exit_status, "curl exit status 18: Partial file. Only a part of the file was transferred.");
			this.checkStatus("200 OK", o.http_status);
			this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
			testlib.equal("12345", o.body);
			testlib.testResult(o.stderr.indexOf("transfer closed with 7 bytes remaining")>=0);
		}
	};

	this.test500Error_invalid_response_generated = function()
	{
		var o;
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				this.rootUrl + "/test/responses/500/invalid-response/undefined");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				this.rootUrl + "/test/responses/500/invalid-response/null");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				this.rootUrl + "/test/responses/500/invalid-response/object");
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				this.rootUrl + "/test/responses/500/invalid-response/int");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				this.rootUrl + "/test/responses/500/invalid-response/string");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/html;charset=UTF-8", o.headers);
	};

	this.test500Error_invalid_response_generated__Post = function()
	{
		var o;
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				this.rootUrl + "/test/responses/500/invalid-response/null");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				this.rootUrl + "/test/responses/500/invalid-response/object");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				this.rootUrl + "/test/responses/500/invalid-response/int");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
		
		o = curl(
				"-H", "From: " + this.userId,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				"-d", "data",
				this.rootUrl + "/test/responses/500/invalid-response/string");
		this.checkStatus("500 Internal Server Error", o.status);
		this.checkHeader("Content-Type", "text/plain;charset=UTF-8", o.headers);
	};
	
	this.testSetBasicCookie = function()
	{
		var url = this.rootUrl + "/test/cookies/basic";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
		this.checkHeader("Set-Cookie", "testCookie=testValue; Max-Age=10", o.headers);
	};
	
	this.testSessionCookie = function()
	{
		var url = this.rootUrl + "/test/cookies/basic";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
		this.cookieWasSet("vvSession", o.headers);
	};
	
	this.testDistinctSessionCookie = function()
	{
		var url = this.rootUrl + "/test/cookies/basic";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				url);
				


		if (!this.checkStatus("200 OK", o.status)) return;
		
		if (!this.checkHeader("Content-Length", "0", o.headers)) return;
		var first_cookie = this.getCookieValue("vvSession", o.headers);

		var o = curl(
				"-H", "From: " + username,
				url);
				

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
		var second_cookie = this.getCookieValue("vvSession", o.headers);
		
		testlib.notEqual(first_cookie, second_cookie, "Recieved non-distinct session cookies");
	};
	
	this.testPassHeader = function()
	{
		var url = this.rootUrl + "/test/headers/x-powered-by";
		var username = this.userId;

		var o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);

		this.checkStatus("200 OK", o.status);
		this.checkHeader("Content-Length", "0", o.headers);
		this.checkHeader("X-Powered-By", "veracity", o.headers);
	};
	
	this.testSessionPersist = function()
	{
		var url = this.rootUrl + "/test/session/persist/test";
		var username = this.userId;
		var obj = { test: true };
		
		var json = JSON.stringify(obj);
		var o = curl(
				"-H", "From: " + username,
				"-d", json,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);
		this.checkStatus("200 OK", o.status);
		
		// Refetch the data
		o = curl(
				"-H", "From: " + username,
				"-b", "cookies.txt",
				"-c", "cookies.txt",
				url);
				

		this.checkStatus("200 OK", o.status);
		var sessionObj = JSON.parse(o.body);

		testlib.testResult(json == o.body, "Session data was persisted", json, o.body);
	};
	
	this.testRepoNamesWithDots = function()
	{
		for (repo in this._testRepos)
		{
			testlib.equal("200 OK", curl(this.rootUrl + "/repos/" + this._testRepos[repo]).status);
			testlib.equal("200 OK", curl(this.rootUrl + "/repos/" + this._testRepos[repo] + ".json").status);
		}
		
	};
}
