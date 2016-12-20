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
//load("vv_serve_process.js");

/* This file contains tests for HTTP push, pull, and clone. */

function suite_web_readonly_normal()
{
	this.suite_setUp = function()
	{
	};

	this.testSimpleRoutes = function()
	{
		var o;

		// GETs against the normal server
		// (Everything should work ("200 OK") since urls should only ever be disabled when in readonly mode.)

		o = curl(this.rootNormalUrl + "/test/readonly/default.txt");
		testlib.equal("200 OK", o.status);

		o = curl(this.rootNormalUrl + "/test/readonly/explicitly-enabled.txt");
		testlib.equal("200 OK", o.status);

		o = curl(this.rootNormalUrl + "/test/readonly/explicitly-disabled.txt");
		testlib.equal("200 OK", o.status);

		// POSTs against the normal server
		// (Everything should work ("200 OK") since urls should only ever be disabled when in readonly mode.)

		o = curl("-d", "data", this.rootNormalUrl + "/test/readonly/default.txt");
		testlib.equal("200 OK", o.status);

		o = curl("-d", "data", this.rootNormalUrl + "/test/readonly/explicitly-enabled.txt");
		testlib.equal("200 OK", o.status);

		o = curl("-d", "data", this.rootNormalUrl + "/test/readonly/explicitly-disabled.txt");
		testlib.equal("200 OK", o.status);
	};
}

