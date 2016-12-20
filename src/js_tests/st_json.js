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

function st_zing_templates()
{
    this.test_empty_string_key = function()
    {
        var t =
        {
            "x" : 5,
            "y" : 7,
            "" : 9
        };

        var s1 = sg.to_json(t);

        var t2 = sg.from_json(s1);

        var s2 = sg.to_json(t2);

        testlib.ok(s1 == s2);
    };

	this.testNull = function()
	{
		
		var json = sg.to_json(null);
		
		testlib.ok(json == "null");
		
		var json = sg.to_json__pretty_print(null);
		
		testlib.ok(json == "null");
	}
	
	this.testBoolean = function()
	{
		
		var json = sg.to_json(true);
		var expected = JSON.stringify(true);
		testlib.ok(json == expected, "true to_json", expected, json);
		
		var json = sg.to_json(false);
		var expected = JSON.stringify(false);
		testlib.ok(json == expected, "false to_json", expected, json);
		
		var json = sg.to_json__pretty_print(true);
		var expected = JSON.stringify(true);
		testlib.ok(json == expected, "true to_json__pretty_print", expected, json);
		
		var json = sg.to_json__pretty_print(false);
		var expected = JSON.stringify(false);
		testlib.ok(json == expected, "false to_json__pretty_print", expected, json);
	}
	
	this.testInteger = function()
	{
		
		var json = sg.to_json(1);
		var expected = JSON.stringify(1);

		testlib.ok(json == expected, "1 to_json", expected, json);
		
		var json = sg.to_json(999);
		var expected = JSON.stringify(999);

		testlib.ok(json == expected, "999 to_json", expected, json);
	}
	
	this.testDouble = function()
	{
		var val = 3.141;
		var json = parseFloat(sg.to_json(val));
		var expected = parseFloat(JSON.stringify(val));

		testlib.ok(json == expected, "1 to_json", expected, json);
	}
	
	this.testTrailingCommaInObjectDefinition = function()
	{
		var json = "{ \"log\" : { \"path\" : \"/tmp/\", }, \"diff\" : { \"2\" : { \"program\" : \"/usr/local/bin/diffmerge\" } } }";
		var expected = { "log": { "path": "/tmp/" }, "diff" : { "2": { "program": "/usr/local/bin/diffmerge" } } };
		
		try
		{
			var out = sg.from_json(json);
			testlib.ok(sg.to_json(out) == sg.to_json(expected), "Objects matched", sg.to_json(expected), sg.to_json(out) )
		}
		catch (err)
		{
			testlib.ok(!err, "Unexpected Error");
		}
	}
	
	this.testTrailingCommaInArrayDefinition = function()
	{
		var json = "{ \"log\" : [ \"/tmp/\", ], \"diff\" : { \"2\" : { \"program\" : \"/usr/local/bin/diffmerge\" } } }";
		var expected = { "log": [ "/tmp/" ], "diff" : { "2": { "program": "/usr/local/bin/diffmerge" } } };
		try
		{
			var out = sg.from_json(json);
			testlib.ok(sg.to_json(out) == sg.to_json(expected), "Objects matched", sg.to_json(expected), sg.to_json(out) )
		}
		catch (err)
		{
			testlib.ok(!err, "Unexpected Error");
		}
	}
}

