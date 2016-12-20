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

//////////////////////////////////////////////////////////////////
// Some basic unit tests to test the sg.config.[...] global properties.
//////////////////////////////////////////////////////////////////

function st_ConfigTests()
{
    this.test_show_version = function()
    {
	// I can't really test what the correct values are.
	// All I can do is print all of the values that I 
	// know about at this time and let you eyeball it.

	print("sglib sg.config.version[...] fields are:");
	print("    [major       " + sg.config.version.major + "]");
	print("    [minor       " + sg.config.version.minor + "]");
	print("    [rev         " + sg.config.version.rev + "]");
	print("    [buildnumber " + sg.config.version.buildnumber + "]");
	print("    [string      " + sg.config.version.string);

	print("sglib sg.config.debug:    " + sg.config.debug);
	print("sglib sg.config.bits:     " + sg.config.bits);
	print("sglib sg.config.platform: " + sg.config.platform);

	print("sglib sg.platform(): " + sg.platform());
	print("sglib sg.version:    " + sg.version);
    }

}
