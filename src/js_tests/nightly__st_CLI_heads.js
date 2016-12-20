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

function st_CLI_heads()
{
	/* Give an error when one of the provided branch names doesn't exist: U4861. */
	this.testBadBranchName = function()
	{
		var o = sg.exec("vv", "heads", "-b", "master");
		print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status == 0);

		o = sg.exec("vv", "heads", "-b", "not-so-fast");
		print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status != 0);

		o = sg.exec("vv", "heads", "-b", "master", "-b", "not-so-fast");
		print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status != 0);

		o = sg.exec("vv", "heads", "-b", "not-so-fast", "-b", "master");
		print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status != 0);

		o = sg.exec("vv", "branch", "add-head", "not-so-fast", "-b", "master");
		print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status == 0);

		o = sg.exec("vv", "heads", "-b", "not-so-fast", "-b", "master");
		print(sg.to_json__pretty_print(o));
        testlib.ok(o.exit_status == 0);
	};
}
