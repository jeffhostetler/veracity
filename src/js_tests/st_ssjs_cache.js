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

function st_ssjs_cache() {
	
	this.setUp = function()
	{
		this.repo = sg.open_repo(repInfo.repoName);
		var serverFiles = sg.local_settings().machine.server.files;
		var core = serverFiles + "/core";
		load(serverFiles + "/dispatch.js");
		load(core + "/vv.js");
		load(core + "/textCache.js");
	};
	
	this.tearDown = function()
	{
		if (this.repo)
			this.repo.close();
	};
	
	this.testLocalSettings = function()
	{
		
	};

	this.testNormalizeQs = function()
	{
		var dags = [];

		var request = 
		{
			requestMethod: "GET",
			uri: "blah",
			queryString: "",
			headers: {},
			repoName: repInfo.repoName,
			repo: this.repo
		};
		
		var cc = new textCache(request, dags);

		testlib.equal("", cc._normalizeQs(""), "empty qs");
		testlib.equal("a=1", cc._normalizeQs("a=1"), "single qs");
		testlib.equal("a=1&b=2", cc._normalizeQs("a=1&b=2"), "alpha qs");
		testlib.equal("a=1&b=2", cc._normalizeQs("b=2&a=1"), "unordered qs");
		testlib.equal("a=1", cc._normalizeQs("a=1&_=ignore"), "ignore _");
		
	};

	this.testIgnoreAjaxCacheNonce = function()
	{
		var dags = [ sg.dagnum.WORK_ITEMS, sg.dagnum.USERS ];
		var url = "/blah";

		var request = 
		{
			requestMethod: "GET",
			uri: url,
			queryString: "",
			headers: {},
			repoName: repInfo.repoName,
			repo: this.repo
		};

		var cacheWithout = new textCache(request, dags);

		request.queryString = "_=1";
		var cacheWith = new textCache(request, dags);

		testlib.testResult(!! cacheWith.etag, "ETag expected");
		testlib.equal(cacheWithout.etag, cacheWith.etag, "ETags should match");
	},

	this.testIgnoreQueryStringOrder = function()
	{
		var dags = [ sg.dagnum.WORK_ITEMS, sg.dagnum.USERS ];
		var url = "/blah";

		var request = 
		{
			requestMethod: "GET",
			uri: url,
			queryString: "a=1&b=2",
			headers: {},
			repoName: repInfo.repoName,
			repo: this.repo
		};

		var cacheAsc = new textCache(request, dags);

		request.queryString = "b=2&a=1";
		var cacheDesc = new textCache(request, dags);

		testlib.testResult(!! cacheAsc.etag, "ETag expected");
		testlib.equal(cacheAsc.etag, cacheDesc.etag, "ETags should match");
	}
}
