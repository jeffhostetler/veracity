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

function st_minify()
{
	this.setUp = function() 
	{
	};

	this.tearDown = function()
	{
	};

	this.testTrivial = function()
	{
		var src = " if (1) alert(2); ";

		var res = sg.jsmin(src);

		testlib.equal("\nif(1)alert(2);", res);
	};

	this.testUni = function()
	{
		var src = " if (1) alert('…'); ";
		var expected = "\n" + src.replace(/ /g, '');

		var res = sg.jsmin(src);

		testlib.equal(expected, res);
	};
}
