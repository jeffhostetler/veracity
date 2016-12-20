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

function st_crash()
{
    this.test_crash = function() 
    {
	if ((sg.platform() == "LINUX") || (sg.platform() == "MAC"))
	{
	    try
	    {
		var o = sg.exec(vv, "crash");

		print(dumpObj(o, "sg.exec(vv, 'crash') yields", "", 0));
	    }
	    catch (eBogus)
	    {
		testlib.ok( (0), "sg.exec() should not have thrown");
		print(dumpObj(eBogus, "sg.exec() threw", "", 0));
	    }
	}
    }
}

