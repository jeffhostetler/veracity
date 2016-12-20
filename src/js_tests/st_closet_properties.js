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

function st_closet_properties()
{
	this.no_setup = true;
	this.oldCloset = "";

	this.setUp = function()
	{
		/* We're deliberately creating a closet within the closet here,
		 * so that we don't create a bunch of new "closet spam:"
		 * deleting .sgcloset-test still cleans everything up.
		 * It works fine today. Someday this could be Wrong. */
		this.oldCloset = sg.getenv("SGCLOSET");
		sg.setenv("SGCLOSET", this.oldCloset + "/" + sg.gid());
		sg.refresh_closet_location_from_environment_for_testing_purposes();
	};

	this.tearDown = function()
	{
		sg.setenv("SGCLOSET", this.oldCloset);
		sg.refresh_closet_location_from_environment_for_testing_purposes();
	};

	this.test_basic = function()
	{
		sg.add_closet_property("test-prop", "12345");
		var prop = sg.get_closet_property("test-prop");
		testlib.testResult(prop == "12345");

		var bCorrectFailure = false;
		try
		{
			sg.add_closet_property("test-prop", "6789");
		}
		catch (e)
		{
			if (e.message.indexOf("Error 292") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
				bCorrectFailure = true;
			else
				print(sg.to_json__pretty_print(e));
		}
		testlib.testResult(bCorrectFailure, "add_closet_property should fail with sglib error 292.");

		var prop2 = sg.get_closet_property("test-prop");
		testlib.testResult(prop2 == "12345");

		sg.set_closet_property("test-prop", "6789");

		var prop3 = sg.get_closet_property("test-prop");
		testlib.testResult(prop3 == "6789");

		bCorrectFailure = false;
		try
		{
			var prop4 = sg.get_closet_property("no-such-prop");
		}
		catch (e)
		{
			if (e.message.indexOf("Error 293") != -1) // This is a pretty weak way to check the failure. We need better exception objects.
				bCorrectFailure = true;
			else
				print(sg.to_json__pretty_print(e));
		}
		testlib.testResult(bCorrectFailure, "get_closet_property should fail with sglib error 293.");
	};
}
