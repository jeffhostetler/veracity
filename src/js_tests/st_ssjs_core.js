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

function st_ssjs_core() {
	
	this.setUp = function()
	{
		this.repo = sg.open_repo(repInfo.repoName);
		var serverFiles = sg.local_settings().machine.server.files;
		var core = serverFiles + "/core";
		load(serverFiles + "/dispatch.js");
		load(core + "/vv.js");
		load(core + "/textCache.js");
		load(core + "/activity.js");
		load(core + "/registerModule.js");
		load(core + "/sync.js");
		load(core + "/vc.js");
		load(core + "/data.js");
		load(core + "/session.js");
		load(core + "/templates.js");
		load(core + "/history.js");
		load(core + "/shell.js");
		load(core + "/test.js");
		load(core + "/local.js");
		load(core + "/strftime.js");

		var db = new zingdb(this.repo, sg.dagnum.USERS);
		var urecs = db.query('user', ['*'], "name == '" + repInfo.userName + "'");

		this.userId = urecs[0].recid;
	};
	
	this.tearDown = function()
	{
		if (this.repo)
			this.repo.close();
	};

	this.testSetDefaultRepo = function()
	{
		var request = {
			'requestMethod': "POST",
			'uri': "/defaultrepo.json",
			'headers': {
				'From': this.userId
			}
		};

		sg.fs.cd("..");
		var oldDefault = getDefaultRepoName(request);

		testlib.ok(!! oldDefault, 'should return something');

		var handler = dispatch(request);
		var newRepoName = sg.gid();

		sg.vv2.init_new_repo( { "repo" : newRepoName, "no-wc" : true } );

		if (! testlib.ok(!! handler.onJsonReceived, "post handler"))
			return;

		var res = handler.onJsonReceived(request, { 'repo': newRepoName } );

		if (! testlib.equal("200 OK", res.statusCode, "return code"))
		{
			vv.log(res);
			return;
		}

		var newDefault = getDefaultRepoName(request);

		testlib.equal(newRepoName, newDefault, "new default repo");
		testlib.notEqual(oldDefault, newDefault, "default should change");
	};

	this.testWhereZero = function() {
		testlib.equal("a == 0", vv.where({'a' : 0}));
	};

	this.testWhereNegation = function() {
		testlib.equal("a != 0", vv.where({'!a': 0}));
		testlib.equal("b != 1", vv.where({'! b': 1}));
	};

	this.testWhereBoolean = function() {
		testlib.equal("a == #FALSE", vv.where({'a': false}));
		testlib.equal("a == #TRUE", vv.where({'a': true}));
		testlib.equal("a == 0", vv.where({'a': 0}));
		testlib.equal("a == '0'", vv.where({'a': '0'}));
		testlib.equal("a == #NULL", vv.where({'a': null}));
		testlib.equal("a == ''", vv.where({'a': ""}));
	};
}
