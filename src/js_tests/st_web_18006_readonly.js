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



load("suite_web_readonly.js");
load("../js_test_lib/vv_serve_process.js");
	
if (testlib.defined.SG_CIRRUS === undefined) function st_web_readonly() {
	var suite = new suite_web_readonly();

	suite.setUp = function () {
		sg.exec('vv', 'config', 'set', 'server/readonly', 'true');
		this.readonly_server_process = new vv_serve_process(18006, { runFromWorkingCopy: true });
		this.rootReadonlyUrl = this.readonly_server_process.url;
		this.suite_setUp();
	};

	suite.tearDown = function () {
	    try
	    {
		sg.exec('vv', 'config', 'reset', 'server/readonly');
	    }
	    finally
	    {
		this.readonly_server_process.stop();
	    }
	};

	return suite;
}

